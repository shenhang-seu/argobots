#ifdef  __cplusplus
extern "C" {
#endif

#include "abti.h"

/*******************************************************************************
    作者: Fang Qiang
        描述: sem句柄转为sem指针
    输入参数: sem句柄
    输出参数: 无
      返回值: sem指针
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
static inline ABTI_sem *ABTI_sem_get_ptr(ABT_sem sem)
{
#ifndef ABT_CONFIG_DISABLE_ERROR_CHECK
    ABTI_sem *p_sem;
    if (sem == ABT_SEM_NULL) {
        p_sem = NULL;
    } else {
        p_sem = (ABTI_sem *)sem;
    }
    return p_sem;
#else
    return (ABTI_sem *)sem;
#endif
}

/*******************************************************************************
    作者: Fang Qiang
        描述: sem指针转为sem句柄
    输入参数: sem指针
    输出参数: 无
      返回值: sem句柄
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
static inline ABT_sem ABTI_sem_get_handle(ABTI_sem *p_sem)
{
#ifndef ABT_CONFIG_DISABLE_ERROR_CHECK
    ABT_sem h_sem;
    if (p_sem == NULL) {
        h_sem = ABT_SEM_NULL;
    } else {
        h_sem = (ABT_sem)p_sem;
    }
    return h_sem;
#else
    return (ABT_sem)p_sem;
#endif
}

/*******************************************************************************
    作者: Fang Qiang
        描述: 创建sem 信号量
    输入参数: newsem, 新建的信号量句柄
    输出参数: 无
      返回值: ABT_SUCCESS
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
int ABT_sem_create(ABT_sem *newsem)
{
    ABTI_UB_ASSERT(ABTI_initialized());
    ABTI_UB_ASSERT(newsem);

    ABTI_sem *p_sem = NULL;

    int abt_errno = ABTU_malloc(sizeof(ABTI_sem), (void **)&p_sem);
    ABTI_CHECK_ERROR(abt_errno);
    ABTD_spinlock_clear(&p_sem->lock);
    p_sem->wait_cnt = 0;
    ABTI_waitlist_init(&p_sem->waitlist);

    *newsem = ABTI_sem_get_handle(p_sem);
    return ABT_SUCCESS;
}

/*******************************************************************************
    作者: Fang Qiang
        描述: 释放sem 信号量
    输入参数: sem, 信号量句柄
    输出参数: 无
      返回值: 无
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void ABT_sem_free(ABT_sem *sem)
{
    ABTI_UB_ASSERT(ABTI_initialized());
    ABTI_UB_ASSERT(sem);

    ABTI_sem *p_sem = ABTI_sem_get_ptr(*sem);
    ABTI_CHECK_NULL_SEM_PTR_ASSERT(p_sem);

    ABTD_spinlock_acquire(&p_sem->lock);
    ABTI_UB_ASSERT(ABTI_waitlist_is_empty(&p_sem->waitlist));

    ABTU_free(p_sem);

    *sem = ABT_SEM_NULL;
    return;
}

/*******************************************************************************
    作者: Fang Qiang
        描述: 初始化sem结构体
    输入参数: 仅仅适用于业务层提供sem 结构体内存
    输出参数: 无
      返回值: 无
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void ABT_sem_init(ABT_sem sem)
{
    ABTI_sem *p_sem = ABTI_sem_get_ptr(sem);
    ABTI_CHECK_NULL_SEM_PTR_ASSERT(p_sem);

    ABTD_spinlock_clear(&p_sem->lock);
    p_sem->wait_cnt = 0;
    ABTI_waitlist_init(&p_sem->waitlist);

    return;
}

/*******************************************************************************
    作者: Fang Qiang
        描述:  信号量等待
    输入参数: sem, 信号量句柄
    输出参数: 无
      返回值: 无
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void ABT_sem_wait(ABT_sem sem)
{
    ABTI_UB_ASSERT(ABTI_initialized());
    ABTI_sem *p_sem = ABTI_sem_get_ptr(sem);
    ABTI_CHECK_NULL_SEM_PTR_ASSERT(p_sem);

    ABTI_local *p_local = ABTI_local_get_local();

    if (ABTI_IS_ERROR_CHECK_ENABLED) {
        ABTI_xstream *p_local_xstream = ABTI_local_get_xstream(p_local);
        ABTI_CHECK_TRUE_ASSERT(p_local_xstream->p_thread->type &
                            ABTI_THREAD_TYPE_YIELDABLE,
                            ABT_ERR_SEM);
    }

    ABTD_spinlock_acquire(&p_sem->lock);
    if ((++p_sem->wait_cnt) > 0) {
        ABTI_waitlist_wait_and_unlock(&p_local, &p_sem->waitlist,
                                      &p_sem->lock,
                                      ABT_SYNC_EVENT_TYPE_SEM,
                                      (void *)p_sem);
    } else {
        ABTD_spinlock_release(&p_sem->lock);
    }
    return;
}


/*******************************************************************************
    作者: Fang Qiang
        描述:  信号量唤醒
    输入参数: sem, 信号量句柄
    输出参数: 无
      返回值: 无
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void ABT_sem_post(ABT_sem sem)
{
    ABTI_UB_ASSERT(ABTI_initialized());
    ABTI_sem *p_sem = ABTI_sem_get_ptr(sem);
    ABTI_CHECK_NULL_SEM_PTR_ASSERT(p_sem);

    ABTI_local *p_local = ABTI_local_get_local();

    ABTD_spinlock_acquire(&p_sem->lock);
    if (p_sem->wait_cnt > 0) {
        ABTI_waitlist_signal(p_local, &p_sem->waitlist);
    }
    p_sem->wait_cnt--;
    ABTD_spinlock_release(&p_sem->lock);
    return;
}


/*******************************************************************************
    作者: Fang Qiang
        描述:  信号量上等待被唤醒的ULT数量
    输入参数: sem, 信号量句柄
    输出参数: 无
      返回值: 等待被唤醒的ULT数量
      注意项: 单线程使用没有问题，多线程使用同一个sem是，此功能不可信
--------------------------------------------------------------------------------
*******************************************************************************/
int ABT_sem_get_wait_ult_cnt(ABT_sem sem)
{
    ABTI_UB_ASSERT(ABTI_initialized());
    ABTI_sem *p_sem = ABTI_sem_get_ptr(sem);
    ABTI_CHECK_NULL_SEM_PTR_ASSERT(p_sem);

    return p_sem->wait_cnt;
}
#ifdef  __cplusplus
}
#endif
