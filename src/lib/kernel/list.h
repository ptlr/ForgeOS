#ifndef LIB_KERNEL_LIST_H
#define LIB_KERNEL_LIST_H

#include "constant.h"
#include "stdint.h"

#define offset(structType, member) (int)(&((structType*)0)->member)
#define elem2entry(structType, structMemberName, elePtr) (structType*)((int)elePtr - offset(structType, structMemberName))
/* 链表节点成员结构
 * 节点不需要数据成员，只需要前驱和后继节点指针
 */
struct ListElem{
    struct ListElem* prev; // 前驱节点
    struct ListElem* next; // 后继节点
};
/* 链表结构，用于实现队列
 */
struct List{
    /* 队首，固定不变，第一个元素是head.next
     */
    struct ListElem head;
    /* 队尾，固定不变
     */
    struct ListElem tail;
};
/* 自定义函数类型function，用于在list_traversal中作回调函数 */
typedef bool Func(struct ListElem*, int);

// 初始化队列
void listInit(struct List*);
// 在元素before前插入元素elem
void listInsertBefore(struct ListElem* before, struct ListElem* elem);
// 在队首压入elem
void listPush(struct List* list, struct ListElem* elem);
// 
//void listIterate(struct List* list);
// 在队尾压入elem
void listAppend(struct List* list, struct ListElem* elem);
// 从队列中移除elem
void listRemove(struct ListElem* elem);
// 从队列中弹出elem
struct ListElem* listPop(struct List* list);
// 查看对了是否为空
bool listIsEmpty(struct List* list);
// 返回list的长度
uint32 listLen(struct List* list);
// 
struct ListElem*  listTraversal(struct List* list, Func func, int arg);
// 查找元素
bool listFind(struct List* list, struct ListElem* elem);
#endif