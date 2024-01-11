#include <stdio.h>
#include <abt.h>
#include <assert.h>

void my_abt_down(void *args) {
    ABT_sem sem = (ABT_sem)args;
    printf("my_abt_down down\n");
    ABT_sem_wait(sem);
    printf("my_abt_down returns from down\n");
}

void my_abt_up(void *args) {
    int *a = ABT_priv_mem_request(sizeof(int));
    assert(NULL != a);
    *a = 8;
    printf("a=%d\n", *a);
    ABT_sem sem = (ABT_sem)args;
    ABT_sem_post(sem);
    printf("my_abt_up\n");
}
/*
int main() {
    ABT_init(0, NULL);

    ABT_sem sem;
    ABT_sem_create(&sem);

    ABT_xstream xstream;
    ABT_xstream_create(ABT_SCHED_NULL, &xstream);

    ABT_thread_create_on_xstream(xstream, my_abt_down, sem, ABT_THREAD_ATTR_NULL, NULL);
    ABT_thread_create_on_xstream(xstream, my_abt_up, sem, ABT_THREAD_ATTR_NULL, NULL);

    // ABT_xstream_free() deallocates the resource used for the execution stream xstream and sets xstream to ABT_XSTREAM_NULL. If xstream is still running, this routine will be blocked on xstream until xstream terminates.
    ABT_xstream_free(&xstream);
    ABT_sem_free(&sem);

    ABT_finalize();

    return 0;
}
*/

int main() {
    ABT_init(0, NULL);

    ABT_sem sem;
    ABT_sem_create(&sem);

    ABT_xstream xstream;
    ABT_xstream_create(ABT_SCHED_NULL, &xstream);

    ABT_thread thread1, thread2;
    ABT_thread_create_on_xstream(xstream, my_abt_down, sem, ABT_THREAD_ATTR_NULL, &thread1);
    ABT_thread_create_on_xstream(xstream, my_abt_up, sem, ABT_THREAD_ATTR_NULL, &thread2);

    ABT_thread_join(thread1);
    ABT_thread_join(thread2);
    ABT_thread_free(&thread1);
    ABT_thread_free(&thread2);

    // ABT_xstream_free() deallocates the resource used for the execution stream xstream and sets xstream to ABT_XSTREAM_NULL. If xstream is still running, this routine will be blocked on xstream until xstream terminates.
    ABT_xstream_free(&xstream);
    ABT_sem_free(&sem);

    ABT_finalize();

    return 0;
}