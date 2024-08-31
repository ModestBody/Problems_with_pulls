if (make_thread && (right - left > 100000)) {
        task_counter->fetch_add(1);  // Увеличиваем счетчик задач
        auto f = pool.push_task(quicksort, std::ref(array), left, j, 
                                task_counter, completion_promise, std::ref(pool), make_thread);
    } else {
        quicksort(array, left, j, task_counter, completion_promise, pool, false);
    }
    quicksort(array, i, right, task_counter, completion_promise, pool, false);

    if (--(*task_counter) == 0) {
        completion_promise->set_value();
    }
}
