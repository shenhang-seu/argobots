#include <stdio.h>
#include <abt.h>

void my_abt_down(void *args) {
    ABT_sem sem = (ABT_sem)args;
    printf("my_abt_down down\n");
    ABT_sem_wait(sem);
    printf("my_abt_down returns from down\n");
}

void my_abt_up(void *args) {
    ABT_sem sem = (ABT_sem)args;
    ABT_sem_post(sem);
    printf("my_abt_up\n");
}

int main() {
    ABT_init(0, NULL);

    ABT_sem sem;
    ABT_sem_create(&sem);

    ABT_xstream xstream;
    ABT_xstream_self(&xstream);

    ABT_thread thread1, thread2;
    ABT_thread_create_on_xstream(xstream, my_abt_down, sem, ABT_THREAD_ATTR_NULL, &thread1);
    ABT_thread_create_on_xstream(xstream, my_abt_up, sem, ABT_THREAD_ATTR_NULL, &thread2);

    ABT_thread_join(thread1);
    ABT_thread_join(thread2);

    ABT_sem_free(&sem);

    ABT_finalize();

    return 0;

}