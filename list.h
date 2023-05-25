//
// Created by Haikal Zain on 24/5/2023.
//

#ifndef TINYJVM_LIST_H
#define TINYJVM_LIST_H


#define list_for_each(el, list_head) for(el = list_head;el!=NULL;el=el->next)
#define list_add(el, list_head) (el->next = list_head)

#endif //TINYJVM_LIST_H
