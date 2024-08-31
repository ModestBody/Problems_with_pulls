if (make_thread && (right_bound - left > 10000)) {
    auto counter = std::make_shared<int>(2); 

    auto f = pool.push_task([counter, array, left, right_bound] {
        if (right_bound - left > 100000) {
            quicksort(array, left, right_bound);
        }
        (*counter)--;
        });

    quicksort(array, left_bound, right);
    (*counter)--;

    f.get();
}
else {
}