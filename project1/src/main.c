#include "index.h"
#include "file.h"
#include "tests.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>

extern int fd;

typedef struct pageNode {
    pagenum_t pageNum;
    struct pageNode * next;
} pageNode;

typedef struct list {
    int* keys;
    int numOfKeys;
} list;

void printFreePageList();
void printHeader(pagenum_t headerPageNum);
void printInternal(pagenum_t internalPageNum);
void printLeaf(pagenum_t leafPageNum);
void printDb();
void printTree();

pageNode* queue = NULL;
void enqueue( pageNode * new_node);
pageNode * dequeue( void );

void listAddKey(list* list, int key);
void listRemoveKey(list* list, int key);
int listGetRandomKey(list* list);
int listIsKeyExist(list* list, int key);

// MAIN
int main( int argc, char ** argv ) {

    open_table("db");
    tests();

    //for test
    list testInput;
    list testInputted;
    
    int testNum;

    srand(time(NULL));

    //test constants
    testNum = 1000;
    int keyRange = testNum * 100;

    testInput.keys = (int*)malloc(sizeof(int) * testNum);
    testInputted.keys = (int*)malloc(sizeof(int) * testNum);
    testInput.numOfKeys = 0;
    testInputted.numOfKeys = 0;
    
    // make testInput 
    int count = testNum;
    int randomKey = rand() % keyRange;
    while (count > 0) {
        if (listIsKeyExist(&testInput, randomKey) == SUCCESS) {
            randomKey = rand() % keyRange;
            continue;
        } else {
            listAddKey(&testInput, randomKey);
            randomKey = rand() % keyRange;
            count--;
        }
    }
    printf("testInput: [");
    for (int i = 0; i < testInput.numOfKeys; i++) {
        printf("%d, ", testInput.keys[i]);
    }
    printf("]\n");

    //do test
    while (testInput.numOfKeys > 0 || testInputted.numOfKeys != 0) {

        // decide input or delete
        if(rand() % 2 == 0) {
            if (testInput.numOfKeys == 0) {
                continue;
            }
            //input
            int key = listGetRandomKey(&testInput);
            printf("[TEST]: Insert %d\n", key);
            db_insert(key, "value");
            listRemoveKey(&testInput, key);
            listAddKey(&testInputted, key);
            printDb();
            printTree();
        } else {
            //delete

            //there's no key in tree
            if (testInputted.numOfKeys == 0) {
                continue;
            } else {
                int key = listGetRandomKey(&testInputted);
                printf("[TEST]: Delete %d\n", key);
                db_delete(key);
                listRemoveKey(&testInputted, key);
                printDb();
                printTree();
            }
        }

    }

    free(testInput.keys);
    free(testInputted.keys);

    //test end

    int input_key;
    char input_value[120];
    char instruction;
    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'i':
            scanf("%d",&input_key);
            scanf("%s",input_value);
            db_insert(input_key, input_value);
            break;
        case 'd':
            scanf("%d",&input_key);
            db_delete(input_key);
            break;
        case 'q':
            close(fd);
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;

        default:
            break;
        }
        while (getchar() != (int)'\n');
        printDb();
        printTree();
        printf("======================================================================\n");
        printf("> ");
    }
    close(fd);
    return EXIT_SUCCESS;
}

void printFreePageList(){
    page_t* page = (page_t*)malloc(sizeof(page_t));
    file_read_page(HEADERPAGENUM, page);

    printf("freePageList: ");
    if(((headerPage_t*)page) -> freePageNum == 0) {
        printf("0\n");
        free(page);
        return;
    } else {
        while(((freePage_t*)page) -> nextFreePageNum != 0) {
            printf("%llu -> ", ((freePage_t*)page) -> nextFreePageNum);
            file_read_page(((freePage_t*)page) -> nextFreePageNum, page);
        }
        printf("0\n");
        free(page);
        return;
    }


}
void printHeader(pagenum_t headerPageNum) {
    page_t* headerPage = (page_t*)malloc(sizeof(struct page_t));
    file_read_page(headerPageNum, headerPage);
    printFreePageList();
    printf("PageNum: %llu\n", headerPageNum);
    printf("HeaderPage\n");
    printf("freePageNum: %llu\nnumOfPages: %llu\nrootPageNum: %llu\n",
    ((headerPage_t*)headerPage) -> freePageNum, ((headerPage_t*)headerPage) -> numOfPages, ((headerPage_t*)headerPage) -> rootPageNum); 
    printf("-------------------------------------------------------------\n");
    free(headerPage);
}
void printInternal(pagenum_t internalPageNum) {
    page_t* internalPage = (page_t*)malloc(sizeof(struct page_t));
    file_read_page(internalPageNum, internalPage);
    printf("PageNum: %llu\n", internalPageNum);
    printf("InternalPage\n");
    printf("internalPage -> parentPageNum: %llu\ninternalPage -> isLeaf: %d\ninternalPage -> numOfKeys: %d\ninternalPage ->leftMostPageNum: %llu\n", ((internalPage_t*)internalPage) -> parentPageNum, ((internalPage_t*)internalPage) -> isLeaf, ((internalPage_t*)internalPage) -> numOfKeys, ((internalPage_t*)internalPage) ->leftMostPageNum);
    for (int i = 0; i < ((internalPage_t*)internalPage) -> numOfKeys; i++) {
        printf("internalPage -> record[%d].key: %lld, internalPage -> record[%d].pageNum: %llu\n", i, ((internalPage_t*)internalPage) -> record[i].key, i, ((internalPage_t*)internalPage) -> record[i].pageNum);
    }
    printf("-------------------------------------------------------------\n");
    free(internalPage);
}
void printLeaf(pagenum_t leafPageNum) {
    page_t* leafPage = (page_t*)malloc(sizeof(struct page_t));
    file_read_page(leafPageNum, leafPage);
    printf("PageNum: %llu\n", leafPageNum);
    printf("LeafPage\n");
    printf("leafPage -> parentPageNum: %llu\nleafPage -> isLeaf: %d\nleafPage -> numOfKeys: %d\nleafPage -> rightSiblingPageNum: %llu\n", ((leafPage_t*)leafPage) -> parentPageNum, ((leafPage_t*)leafPage) -> isLeaf, ((leafPage_t*)leafPage) -> numOfKeys, ((leafPage_t*)leafPage) -> rightSiblingPageNum);
    for (int i = 0; i < ((leafPage_t*)leafPage) -> numOfKeys; i++) {
        printf("leafPage -> record[%d].key: %lld, leafPage -> record[%d].value: %s\n", i, ((leafPage_t*)leafPage) -> record[i].key, i, ((leafPage_t*)leafPage) -> record[i].value);
    }
    printf("-------------------------------------------------------------\n");
    free(leafPage);
}

void enqueue( pageNode * new_node ) {
    pageNode * c;
    if (queue == NULL) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}
pageNode * dequeue( void ) {
    pageNode * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}

void printTree() {
    page_t header;
    file_read_page(HEADERPAGENUM, &header);
    pagenum_t rootPageNum = ((headerPage_t*)&header) ->rootPageNum;
    printf("=======================================PRINTTREE=======================================\n");
    pageNode root = {.pageNum = rootPageNum, .next = NULL};

    printHeader(HEADERPAGENUM);
    if (rootPageNum == 0) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(&root);
    while( queue != NULL ) {
        pageNode* node = dequeue();
        pagenum_t pageNum = node -> pageNum;
        page_t* page = (page_t*)malloc(sizeof(page_t));
        file_read_page(pageNum, page);

        if (!(((internalPage_t*)page) -> isLeaf)) {
            printInternal(pageNum);
            pageNode* newNode = (pageNode*)malloc(sizeof(struct pageNode));
            newNode -> pageNum = ((internalPage_t*)page) -> leftMostPageNum;
            newNode -> next = NULL;
            enqueue(newNode);
            for (int i = 0; i < ((internalPage_t*)page) -> numOfKeys; i++) {
                newNode = (pageNode*)malloc(sizeof(struct pageNode));
                newNode -> pageNum = ((internalPage_t*)page) -> record[i].pageNum;
                newNode -> next = NULL;
                enqueue(newNode);
            }
        } else {
            printLeaf(pageNum);
        }
    }
    printf("=======================================PRINTTREEEND=======================================\n");
}

void printDb() {
    printf("=======================================PRINTDB=======================================\n");
    page_t* page = (page_t*)malloc(sizeof(struct page_t));
    pagenum_t pageNum = 0;
    uint64_t numOfPages;

    printHeader(HEADERPAGENUM);
    int listIndex = 0;
    file_read_page(HEADERPAGENUM, page);
    pageNum = ((headerPage_t*)page) -> rootPageNum;
    pagenum_t rootPageNum = pageNum;

    uint64_t numofpages = ((headerPage_t*)page) -> numOfPages;
    while (pageNum < numofpages) {
        file_read_page(pageNum, page);
        if (((internalPage_t*)page) -> isLeaf) {
            printf("pageNum: %llu\n", pageNum);
            printLeaf(pageNum);
        } else {
            printf("pageNum: %llu\n", pageNum);
            printInternal(pageNum);
        }
        pageNum++;
    }
    
    

    free(page);
    printf("=======================================PRINTDBEND=======================================\n");
    

}

void listAddKey(list* list, int key) {
    list -> keys[list -> numOfKeys] = key;
    list -> numOfKeys++;
}

void listRemoveKey(list* list, int key) {
    int i;
    if (listIsKeyExist(list, key) == FAIL) {
        printf("no key in list\n");
        return;
    }

    for (i = 0; i < list -> numOfKeys; i++) {
        if (list -> keys[i] == key) {
            break;
        }
    }
    for (; i < list -> numOfKeys - 1; i++) {
        list -> keys[i] = list -> keys[i + 1];
    }
    list -> numOfKeys--;
}

int listGetRandomKey(list* list) {
    return list -> keys[rand() % list -> numOfKeys];
};

int listIsKeyExist(list* list, int key) {
    for (int i = 0; i < list -> numOfKeys; i++) {
        if (list -> keys[i] == key) {
            return SUCCESS;
        }
    }
    return FAIL;
}