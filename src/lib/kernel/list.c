#include "list.h"
#include "interrupt.h"
// 初始化队列
void listInit(struct List* list){
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}
// 在元素before前插入元素elem
void listInsertBefore(struct ListElem* before, struct ListElem* elem){
    // 需要关中断
    enum IntrStatus oldStatus = intrDisable();
    // 将before的前驱元素的后继元素设置成elem,此时，before脱离链表
    before->prev->next = elem;
    /* 更新elem的前驱设置成before的前驱节点
     * 更新elem的后驱节点为before,此时，before又回到链表中
     */
    elem->prev = before->prev;
    elem->next = before;
    // 更新befor的前驱节点为elem，此时，双向都链接上
    before->prev = elem;
    setIntrStatus(oldStatus);
}
// 在队首压入elem
void listPush(struct List* list, struct ListElem* elem){
    listInsertBefore(list->head.next, elem); // 队首插入elem
}
/* 
void listIterate(struct List* list){

}*/
// 在队尾压入elem
void listAppend(struct List* list, struct ListElem* elem)
{
    listInsertBefore(&list->tail, elem); // 队尾插入elem
}
// 从队列中移除elem
void listRemove(struct ListElem* elem){
    enum IntrStatus oldStatus = intrDisable();
    elem->prev->next = elem->next;
    elem->next->prev = elem->prev;

    setIntrStatus(oldStatus);
}
// 从队列中弹出elem
struct ListElem* listPop(struct List* list)
{
    struct ListElem* elem = list->head.next;
    listRemove(elem);
    return elem;
}
// 查看对了是否为空
bool listIsEmpty(struct List* list){
    return (list->head.next == &list->tail ? true : false);
}
// 返回list的长度
uint32 listLen(struct List* list){
    struct ListElem* elem = list->head.next;
    uint32 length = 0;
    while(elem != &list->tail){
        length++;
        elem = elem->next;
    }
    return length;
}
/* 把列表中的每个elem和arg传递给回调函数func
 * 如过有条件匹配的返回元素指针，否则返回NULL
 */
struct ListElem*  listTraversal(struct List* list, Func func, int arg){
    struct ListElem* elem = list->head.next;
    // 队列为空时直接返回NULL
    if(listIsEmpty(list)) return NULL;
    while(elem != &list->tail){
        if(func(elem, arg)) return elem;
        elem = elem->next;
    }
    return NULL;
}
// 查找元素
bool listFind(struct List* list, struct ListElem* elem){
    struct ListElem* mElem = list->head.next;
    while (mElem != &list->tail)
    {
        if(mElem == elem) return true;
        mElem = mElem->next;
    }
    return false;
}