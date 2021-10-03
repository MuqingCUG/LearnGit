#include"string.h"
#include"stdio.h"
#include"stdlib.h"

//define node of list
struct list_node
{
    /* data */
    int data;
    struct list_node *next;
};

typedef struct list_node list_single;

list_single *create_list_node(int data)
{
	list_single *node = malloc(sizeof(list_single));
	if(node == NULL){
		printf("malloc fair!\n");
	}
	memset(node,0,sizeof(list_single));
	node->data = data;
	node->next = NULL ;
	return node ;
}

void tail_insert(list_single* pH, list_single* new)
{
    while (pH->next != NULL)
    {
        /* code */
        pH = pH->next;
    }
    pH->next = new;
}

void Print_node(list_single *pH)
{
	//获取当前的位置 
	list_single *p = pH ;
	//获取第一个节点的位置 
	p = p->next ;
	//如果当前位置的下一个节点不为空 
	while(NULL != p->next)
	{
		//(1)打印节点的数据 
		printf("id:%d\n",p->data);
		//(2)移动到下一个节点,如果条件仍为真，则重复(1)，再(2) 
		p = p->next ;
	}
	//如果当前位置的下一个节点为空，则打印数据
	//说明只有一个节点 
	printf("id:%d\n",p->data);
}