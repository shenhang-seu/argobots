/*******************************************************************************
--------------------------------------------------------------------------------
            thread_priv_mem.c
工程代码: 
模块名字: 协程uthread
创建时间: 2023-10-17
作者: 
描述: 支持业务层向ULT申请内存。
    ULT向系统申请4K/指定 大内存, 按照业务层的实际需求分配给业务层使用
    ULT生命周期结束后, 自动释放UTL的所有4K/其他内存
    仅适用于非长驻存ULT多次申请小内存的场景
--------------------------------------------------------------------------------
*******************************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

#include "abti.h"


/*******************************************************************************
    作者: 
        描述: 用于协程thread 向系统申请一个新的page内存
    输入参数: 参数p_thread, 协程thread指针
    输出参数: 无
      返回值: void指针, 返回向系统申请的page首地址
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void * ABTI_thread_malloc_page(ABTI_thread *p_thread)
{
    struct ABTI_priv_page *priv_new_page = NULL;
    int abt_errno= ABTU_malloc(p_thread->priv_page_size, (void **)&priv_new_page);
    ABTI_ASSERT(abt_errno == 0);
    if (abt_errno) {
        printf("ULT %p failed to malloc memory from the system.\n", p_thread);
        return NULL;
    }

    /* priv_page_head 最开始是初始化为NULL */
    priv_new_page->next  = p_thread->priv_page_head;
    p_thread->priv_page_head = priv_new_page;
    p_thread->priv_cur_page_idx = ABTI_PRIV_HEAD_SIZE;
    p_thread->priv_page_num++;

#ifdef ABT_CONFIG_STATIS_COUNT
    ABTI_global *p_global;
    ABTI_SETUP_GLOBAL_ASSERT(&p_global);
    if (p_thread->priv_page_num > p_global->ult_warn_pmp_num ) {
        printf("Priv mem pages(%lu) malloc by ULT(%p) is greater than expected(%lu).",
                p_thread->priv_page_num, p_thread, p_global->ult_warn_pmp_num);
    }

    ABTI_pool *p_pool = p_thread->p_pool;
    ABTI_ASSERT(p_pool);
    p_pool->malloc_page_num++;
#endif

    return priv_new_page;
}


/*******************************************************************************
    作者: 
        描述: 用于业务层申请协程私有内存
    输入参数: 参数len, 业务层申请协程私有内存大小
    输出参数: 无
      返回值: void指针, 返回业务层申请的协程内存地址
      注意项: 当向系统申请内存失败时, 返回NULL指针
--------------------------------------------------------------------------------
*******************************************************************************/
void* ABT_priv_mem_request(uint32_t len)
{
    ABTI_UB_ASSERT(len != 0);
    ABTI_xstream *p_local_xstream = ABTI_local_get_xstream(ABTI_local_get_local());
    ABTI_ASSERT(p_local_xstream);

    ABTI_thread *p_self_thread = p_local_xstream->p_thread;
    ABTI_ASSERT(p_self_thread);

    /*不限制只能使用4M，但限制超过最大申请大小后assert*/
    if ((len  + ABTI_PRIV_HEAD_SIZE) > p_self_thread->priv_page_size) {
        assert(0);
        return NULL;
    }

    ABTI_ASSERT((p_self_thread->priv_page_head == NULL &&  p_self_thread->priv_page_num ==0) ||
            (p_self_thread->priv_page_head &&  p_self_thread->priv_page_num));

    /* 首次申请内存时priv_page_head == NULL &&  priv_cur_page_idx == page_size */
    if (p_self_thread->priv_cur_page_idx + len > p_self_thread->priv_page_size) {
        /*申请内存失败后，返回NULL*/
        if (NULL == ABTI_thread_malloc_page(p_self_thread)) {
            return NULL;
        }
    }

   void *ret_buf = p_self_thread->priv_page_head + p_self_thread->priv_cur_page_idx;
    p_self_thread->priv_cur_page_idx += len;

    return ret_buf;
}


/*******************************************************************************
    作者: 
        描述: 用于ULT生命周期结束后, 释放ULT的所有Page内存
    输入参数: 参数p_thread, 执行thread指针
    输出参数: 无
      返回值: 无
      注意项: 无
--------------------------------------------------------------------------------
*******************************************************************************/
void ABTI_priv_mem_free(ABTI_thread *p_thread)
{
    ABTI_ASSERT(p_thread);
    struct ABTI_priv_page *priv_page_tmp = NULL;
    ABTI_pool *p_pool = p_thread->p_pool;
    ABTI_ASSERT(p_pool);

    while (p_thread->priv_page_head) {
        priv_page_tmp = p_thread->priv_page_head;
        p_thread->priv_page_head = priv_page_tmp->next;
        ABTU_free(priv_page_tmp);
        printf("ABTI_priv_mem_free中调用ABTU_free\n");
        ABTI_ASSERT(p_thread->priv_page_num > 0);
        p_thread->priv_page_num--;

#ifdef ABT_CONFIG_STATIS_COUNT
        p_pool->free_page_num++;
#endif
    }

    ABTI_ASSERT(p_thread->priv_page_num == 0);

    p_thread->priv_page_head = NULL;
    p_thread->priv_cur_page_idx = 0;
}

#ifdef  __cplusplus
}
#endif
