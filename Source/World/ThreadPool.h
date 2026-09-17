#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

using namespace std;

/*
	a small persistent worker pool for blocking parallel_for calls.

	the workers are created once and reused: spawning a thread per chunk costs
	roughly as much as generating one, which would cancel out the gain. the
	calling thread also takes items, so an N-worker pool uses N+1 cores.
*/
class ThreadPool {
public:
	explicit ThreadPool(unsigned worker_count) {
		for (unsigned i = 0; i < worker_count; ++i) {
			workers.emplace_back([this] { worker_loop(); });
		}
	}

	~ThreadPool() {
		{
			lock_guard<mutex> lock(m);
			stopping = true;
		}
		cv.notify_all();
		for (thread& t : workers) {
			if (t.joinable()) t.join();
		}
	}

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	//runs fn(0..count-1) across the pool and returns once every item is done
	void parallel_for(int count, const function<void(int)>& fn) {
		if (count <= 0) return;
		if (workers.empty()) {
			for (int i = 0; i < count; ++i) fn(i);
			return;
		}

		{
			lock_guard<mutex> lock(m);
			job = &fn;
			total = count;
			next_index = 0;
			completed = 0;
		}
		cv.notify_all();

		//the caller works too rather than idling while the pool runs
		while (true) {
			unique_lock<mutex> lock(m);
			if (next_index >= total) break;
			int index = next_index++;
			lock.unlock();

			fn(index);

			lock.lock();
			++completed;
		}

		unique_lock<mutex> lock(m);
		finished_cv.wait(lock, [this] { return completed >= total; });
		job = nullptr;
		total = 0;
	}

private:
	void worker_loop() {
		while (true) {
			unique_lock<mutex> lock(m);
			cv.wait(lock, [this] { return stopping || next_index < total; });
			if (stopping) return;

			int index = next_index++;
			const function<void(int)>* current = job;
			lock.unlock();

			(*current)(index);

			lock.lock();
			bool last = (++completed >= total);
			lock.unlock();
			if (last) finished_cv.notify_one();
		}
	}

	vector<thread> workers;
	mutex m;
	condition_variable cv;
	condition_variable finished_cv;
	const function<void(int)>* job = nullptr;
	int total = 0;
	int next_index = 0;
	int completed = 0;
	bool stopping = false;
};
