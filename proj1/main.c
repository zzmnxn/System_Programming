#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "hash.h"
#include "bitmap.h"

#define MAX_OBJECTS 10
#define MAX_ARGC 10
#define CMD_LEN 100

// 새로 추가한 list_item 구조체: list와 hash에서 데이터를 저장할 구조체
struct list_item
{
    struct list_elem elem;
    struct hash_elem hash_elem; 
    int data;
};

// list, hash, bitmap 객체 배열 선언
struct list lists[MAX_OBJECTS];
struct hash hashtables[MAX_OBJECTS];
struct bitmap *bitmaps[MAX_OBJECTS];

//해당 인덱스 구조체 있는지 확인 
bool list_created[MAX_OBJECTS] = { false };
bool hash_created[MAX_OBJECTS] = { false };
bool bm_created[MAX_OBJECTS] = { false };

// 리스트 비교 함수 
bool less_func(const struct list_elem *a, const struct list_elem *b, void *aux)
{

    struct list_item *item_a = list_entry(a, struct list_item, elem);
    struct list_item *item_b = list_entry(b, struct list_item, elem);
    return item_a->data < item_b->data;
}
// 해시 함수: 데이터 값을 기준으로 해시 생성
unsigned hash_func(const struct hash_elem *e, void *aux)
{

    struct list_item *item = hash_entry(e, struct list_item, hash_elem);
    return hash_int(item->data);
}
// 해시 비교 함수
bool main_hash_less(const struct hash_elem *a, const struct hash_elem *b, void *aux)
{

    struct list_item *item_a = hash_entry(a, struct list_item, hash_elem);
    struct list_item *item_b = hash_entry(b, struct list_item, hash_elem);
    return item_a->data < item_b->data;
}
// hash_apply: 데이터 값을 세제곱으로 변경
void triple_apply(struct hash_elem *e, void *aux)
{

    struct list_item *item = hash_entry(e, struct list_item, hash_elem);
    item->data = item->data * item->data * item->data;
}
// hash_apply: 데이터 값을 제곱으로 변경
void square_apply(struct hash_elem *e, void *aux)
{

    struct list_item *item = hash_entry(e, struct list_item, hash_elem);
    item->data *= item->data;
}
// 문자열 == "true"?
bool is_str_true(const char *s)
{
    return strcmp(s, "true") == 0;
}
// 해시 요소 메모리 해제용
void elem_destructor(struct hash_elem *e, void *aux)
{

    free(hash_entry(e, struct list_item, hash_elem));
}
// 객체 타입 구분용용
enum obj_type
{
    OBJ_LIST,
    OBJ_HASH,
    OBJ_BM
};
// 객체 이름에서 타입과 인덱스 추출
int parse_object_index(const char *name, enum obj_type *type)
{
    int idx;
    if (sscanf(name, "list%d", &idx) == 1)
    {
        *type = OBJ_LIST;
        return idx;
    }
    else if (sscanf(name, "hash%d", &idx) == 1)
    {
        *type = OBJ_HASH;
        return idx;
    }
    else if (sscanf(name, "bm%d", &idx) == 1)
    {
        *type = OBJ_BM;
        return idx;
    }
    return -1;
}
// 출력용 함수 3개
void dump_list(int idx)
{
    struct list_elem *e = list_begin(&lists[idx]);
    while (e != list_end(&lists[idx]))
    {
        struct list_item *item = list_entry(e, struct list_item, elem);
        printf("%d ", item->data);
        e = list_next(e);
    }
    printf("\n");
    fflush(stdout); // 혹시 버퍼링 때문에 출력 안 되는 경우 방지
}

void dump_hash(int idx)
{
    struct hash_iterator it;
    hash_first(&it, &hashtables[idx]);
    while (hash_next(&it))
    {
        struct list_item *item = hash_entry(hash_cur(&it), struct list_item, hash_elem);
        printf("%d ", item->data);
    }
    printf("\n");
}

void dump_bm(int idx)
{
    struct bitmap *bm = bitmaps[idx];
    for (size_t i = 0; i < bitmap_size(bm); i++)
    {
        printf("%d", bitmap_test(bm, i));
    }
    printf("\n");
}

int main()
{
    char command[CMD_LEN];

    while (1)
    {
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;

        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "quit") == 0)
            break;

        // 명령어 파싱
        char *argv[MAX_ARGC];
        int argc = 0;
        char *ptr = strtok(command, " ");
        while (ptr && argc < MAX_ARGC)
        {
            argv[argc++] = ptr;
            ptr = strtok(NULL, " ");
        }
        if (argc == 0)
            continue;
        // create 명령어 처리
        if (strcmp(argv[0], "create") == 0)
        {
            enum obj_type type;
            int idx = parse_object_index(argv[2], &type);
            if (type == OBJ_LIST && !list_created[idx])
            {
                list_init(&lists[idx]);
                list_created[idx]=true;
            }
            else if (type == OBJ_HASH && !hash_created[idx])
            {
                hash_init(&hashtables[idx], hash_func, main_hash_less, NULL);
                hash_created[idx]=true;
            }
            if (type == OBJ_BM && !bm_created[idx])
            {
                int size = atoi(argv[3]);
                bitmaps[idx] = bitmap_create(size);
                bm_created[idx]=true;
            }
        }
        // delete 명령어 처리
        else if (strcmp(argv[0], "delete") == 0 && argc == 2)
        {
            enum obj_type type;
            int idx = parse_object_index(argv[1], &type);
            if (type == OBJ_LIST && list_created[idx])
            {
                for (struct list_elem *e = list_begin(&lists[idx]); e != list_end(&lists[idx]);)
                {
                    struct list_elem *next = list_next(e);
                    struct list_item *item = list_entry(e, struct list_item, elem);
                    free(item);
                    e = next;
                }

                list_init(&lists[idx]);
                list_created[idx] = false;

            }
            else if (type == OBJ_HASH && hash_created[idx])
            {
                hash_destroy(&hashtables[idx], elem_destructor);
                hash_created[idx]=false;
            }
            else if (type == OBJ_BM && bm_created[idx])
            {
                bitmap_destroy(bitmaps[idx]);
                bitmaps[idx] = NULL;
                bm_created[idx]=false;
            }
        }
        // dumpdata 명령 처리
        else if (strcmp(argv[0], "dumpdata") == 0 && argc == 2)
        {
            enum obj_type type;
            int idx = parse_object_index(argv[1], &type);
            if (type == OBJ_LIST&& list_created[idx])
                dump_list(idx);
            else if (type == OBJ_HASH&& hash_created[idx])
                dump_hash(idx);
            else if (type == OBJ_BM&& bm_created[idx])
                dump_bm(idx);
        }
        // list_ 관련 명령어 처리
        else if (strncmp(argv[0], "list_", 5) == 0)
        {
            enum obj_type type;
            int idx = parse_object_index(argv[1], &type);
            if (strcmp(argv[0], "list_push_front") == 0 && argc == 3)
            {
                int val = atoi(argv[2]);
                if (idx >= 0)
                {
                    struct list_item *item = malloc(sizeof(struct list_item));
                    item->data = val;
                    list_push_front(&lists[idx], &item->elem);
                }
            }
            else if (strcmp(argv[0], "list_push_back") == 0 && argc == 3)
            {
                int val = atoi(argv[2]);
                if (idx >= 0)
                {
                    struct list_item *item = malloc(sizeof(struct list_item));
                    item->data = val;
                    list_push_back(&lists[idx], &item->elem);
                }
            }
            else if (strcmp(argv[0], "list_pop_front") == 0 && argc == 2)
            {
                if (idx >= 0 && !list_empty(&lists[idx]))
                {
                    struct list_elem *e = list_pop_front(&lists[idx]);
                    free(list_entry(e, struct list_item, elem));
                }
            }
            else if (strcmp(argv[0], "list_pop_back") == 0 && argc == 2)
            {
                if (idx >= 0 && !list_empty(&lists[idx]))
                {
                    struct list_elem *e = list_pop_back(&lists[idx]);
                    free(list_entry(e, struct list_item, elem));
                }
            }
            else if (strcmp(argv[0], "list_front") == 0 && argc == 2)
            {
                if (idx >= 0)
                    printf("%d\n", list_entry(list_front(&lists[idx]), struct list_item, elem)->data);
            }
            else if (strcmp(argv[0], "list_back") == 0 && argc == 2)
            {
                if (idx >= 0)
                    printf("%d\n", list_entry(list_back(&lists[idx]), struct list_item, elem)->data);
            }
            else if (strcmp(argv[0], "list_empty") == 0 && argc == 2)
            {
                if (idx >= 0)
                    printf("%s\n", list_empty(&lists[idx]) ? "true" : "false");
            }
            else if (strcmp(argv[0], "list_size") == 0 && argc == 2)
            {
                if (idx >= 0)
                    printf("%zu\n", list_size(&lists[idx]));
            }
            else if (strcmp(argv[0], "list_max") == 0 && argc == 2)
            {
                struct list_elem *e = list_max(&lists[idx], less_func, NULL);
                printf("%d\n", list_entry(e, struct list_item, elem)->data);
            }
            else if (strcmp(argv[0], "list_min") == 0 && argc == 2)
            {
                struct list_elem *e = list_min(&lists[idx], less_func, NULL);
                printf("%d\n", list_entry(e, struct list_item, elem)->data);
            }
            else if (strcmp(argv[0], "list_sort") == 0 && argc == 2)
            {
                if (idx >= 0)
                    list_sort(&lists[idx], less_func, NULL);
            }
            else if (strcmp(argv[0], "list_reverse") == 0 && argc == 2)
            {
                if (idx >= 0)
                    list_reverse(&lists[idx]);
            }
            else if (strcmp(argv[0], "list_shuffle") == 0 && argc == 2)
            {
                if (idx >= 0)
                    list_shuffle(&lists[idx]);
            }
            else if (strcmp(argv[0], "list_swap") == 0 && argc == 4)
            {

                int i1 = atoi(argv[2]), i2 = atoi(argv[3]);
                struct list_elem *e1 = list_begin(&lists[idx]);
                struct list_elem *e2 = list_begin(&lists[idx]);
                for (int i = 0; i < i1; i++)
                    e1 = list_next(e1);
                for (int i = 0; i < i2; i++)
                    e2 = list_next(e2);
                list_swap(e1, e2);
            }
            else if (strcmp(argv[0], "list_splice") == 0 && argc == 6)
            {
                int to = parse_object_index(argv[1], &type);
                int before_idx = atoi(argv[2]);
                int from = parse_object_index(argv[3], &type);
                int first_idx = atoi(argv[4]);
                int count = atoi(argv[5]);
                // from 리스트에서 first_idx부터 count 개수만큼의 요소들을
                // to 리스트의 before_idx 위치에 삽입함

                struct list_elem *before = list_begin(&lists[to]);
                struct list_elem *first = list_begin(&lists[from]);
                struct list_elem *last = first;

                for (int i = 0; i < before_idx; i++)
                    before = list_next(before);
                for (int i = 0; i < first_idx; i++)
                    first = list_next(first);
                for (int i = 0; i < count && last != list_end(&lists[from]); i++)
                    last = list_next(last);

                list_splice(before, first, last);
            }
            else if (strcmp(argv[0], "list_unique") == 0)
            {
                // 인자가 3개일 떄떄
                if (argc == 3)
                {
                    int a = parse_object_index(argv[1], &type);
                    int b = parse_object_index(argv[2], &type);
                    // 먼저 정렬된 상태 만들기
                    list_sort(&lists[a], less_func, NULL);
                    list_unique(&lists[a], &lists[b], less_func, NULL);
                }
                // 인자가 2개일 때
                else if (argc == 2)
                {
                    int a = parse_object_index(argv[1], &type);
                    if (type == OBJ_LIST && a >= 0 && list_created[a])
                    {
                        // 먼저 정렬된 상태 만들기기
                        list_sort(&lists[a], less_func, NULL);
                        list_unique(&lists[a], NULL, less_func, NULL);
                    }
                }
            }
            else if (strcmp(argv[0], "list_insert") == 0 && argc == 4)
            {
                int idx = parse_object_index(argv[1], &type);
                int pos = atoi(argv[2]);
                int val = atoi(argv[3]);
                if (idx >= 0 && pos >= 0)
                {
                    struct list_item *item = malloc(sizeof(struct list_item));
                    item->data = val;
                    struct list_elem *target = list_begin(&lists[idx]);
                    for (int i = 0; i < pos && target != list_end(&lists[idx]); i++)
                        target = list_next(target);
                    list_insert(target, &item->elem);
                }
            }
            else if (strcmp(argv[0], "list_insert_ordered") == 0 && argc == 3)
            {
                int val = atoi(argv[2]);
                if (idx >= 0)
                {
                    struct list_item *item = malloc(sizeof(struct list_item));
                    item->data = val;
                    list_insert_ordered(&lists[idx], &item->elem, less_func, NULL);
                }
            }
            else if (strcmp(argv[0], "list_remove") == 0 && argc == 3)
            {
                int pos = atoi(argv[2]);
                if (idx >= 0 && pos >= 0 && !list_empty(&lists[idx]))
                {
                    struct list_elem *target = list_begin(&lists[idx]);
                    for (int i = 0; i < pos && target != list_end(&lists[idx]); i++)
                        target = list_next(target);
                    if (target != list_end(&lists[idx]))
                    {
                        list_remove(target);
                        struct list_item *item = list_entry(target, struct list_item, elem);
                        free(item);
                    }
                }
            }
        }
        // 해시 관련 명령어 처리
        else if (strncmp(argv[0], "hash_", 5) == 0)
        {
            int idx;
            if (sscanf(argv[1], "hash%d", &idx) != 1)
            {
                printf("invalid hash index.\n");
                return 0;
            }
            struct hash *ht = &hashtables[idx];
            if (strcmp(argv[0], "hash_insert") == 0 && argc == 3)
            {
                struct list_item *item = malloc(sizeof(struct list_item));
                item->data = atoi(argv[2]);
                hash_insert(ht, &item->hash_elem);
            }
            else if (strcmp(argv[0], "hash_replace") == 0 && argc == 3)
            {
                struct list_item *item = malloc(sizeof(struct list_item));
                item->data = atoi(argv[2]);
                hash_replace(ht, &item->hash_elem);
            }
            else if (strcmp(argv[0], "hash_find") == 0 && argc == 3)
            {
                // 찾고자 하는 값을 가진 임시 item 생성
                struct list_item *temp = malloc(sizeof(struct list_item));
                temp->data = atoi(argv[2]);
                struct hash_elem *found = hash_insert(ht, &temp->hash_elem);
                // insert가 가능하다면(이미 같은 값 존재재) found는 기존 요소를 반환환
                if (found != NULL)
                {
                    struct list_item *item = hash_entry(found, struct list_item, hash_elem);
                    printf("%d\n", item->data);
                    free(temp);
                }
                else
                {
                    // 새로운 값이 삽입된 경우 -> 삭제하고 출력 없음
                    hash_delete(ht, &temp->hash_elem);
                    free(temp);
                }
            }
            else if (strcmp(argv[0], "hash_delete") == 0 && argc == 3)
            {
                struct list_item temp;
                temp.data = atoi(argv[2]);
                struct hash_elem *deleted = hash_delete(ht, &temp.hash_elem);
                if (deleted != NULL)
                    free(hash_entry(deleted, struct list_item, hash_elem));
            }
            else if (strcmp(argv[0], "hash_apply") == 0 && argc == 3)
            {
                if (strcmp(argv[2], "triple") == 0)
                {
                    hash_apply(ht, triple_apply);
                }
                if (strcmp(argv[2], "square") == 0)
                {
                    hash_apply(ht, square_apply);
                }
            }
            else if (strcmp(argv[0], "hash_clear") == 0 && argc == 2)
            {
                hash_clear(ht, elem_destructor);
            }
            else if (strcmp(argv[0], "hash_empty") == 0 && argc == 2)
            {
                printf("%s\n", hash_empty(ht) ? "true" : "false");
            }
            else if (strcmp(argv[0], "hash_size") == 0 && argc == 2)
            {
                printf("%zu\n", hash_size(ht));
            }
        }
        // 비트맵 관련 명령어 처리
        else if (strncmp(argv[0], "bitmap_", 7) == 0)
        {
            enum obj_type type;
            int idx = parse_object_index(argv[1], &type);
            if (type == OBJ_BM && idx >= 0 && bm_created[idx] )
            {
                struct bitmap *bm = bitmaps[idx];
                if (strcmp(argv[0], "bitmap_mark") == 0 && argc == 3)
                {
                    int bit_idx = atoi(argv[2]);
                    bitmap_mark(bm, bit_idx);
                }
                else if (strcmp(argv[0], "bitmap_reset") == 0 && argc == 3)
                {
                    int bit_idx = atoi(argv[2]);
                    bitmap_reset(bm, bit_idx);
                }
                else if (strcmp(argv[0], "bitmap_flip") == 0 && argc == 3)
                {
                    int bit_idx = atoi(argv[2]);
                    bitmap_flip(bm, bit_idx);
                }
                else if (strcmp(argv[0], "bitmap_test") == 0 && argc == 3)
                {
                    int bit_idx = atoi(argv[2]);
                    printf("%s\n", bitmap_test(bm, bit_idx) ? "true" : "false");
                }
                else if (strcmp(argv[0], "bitmap_size") == 0 && argc == 2)
                    printf("%zu\n", bitmap_size(bm));
                else if (strcmp(argv[0], "bitmap_all") == 0 && argc == 4)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    printf("%s\n", bitmap_all(bm, start, count) ? "true" : "false");
                }
                else if (strcmp(argv[0], "bitmap_any") == 0 && argc == 4)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    printf("%s\n", bitmap_any(bm, start, count) ? "true" : "false");
                }
                else if (strcmp(argv[0], "bitmap_none") == 0 && argc == 4)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    printf("%s\n", bitmap_none(bm, start, count) ? "true" : "false");
                }
                else if (strcmp(argv[0], "bitmap_set_all") == 0 && argc == 3)
                {
                    bool val = strcmp(argv[2], "true") == 0;
                    bitmap_set_all(bm, val);
                }
                else if (strcmp(argv[0], "bitmap_set") == 0 && argc == 4)
                {
                    int bit_idx = atoi(argv[2]);
                    bool val = strcmp(argv[3], "true") == 0;
                    bitmap_set(bm, bit_idx, val);
                }
                else if (strcmp(argv[0], "bitmap_contains") == 0 && argc == 5)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    // 입력 받은 문자열을 bool로 변환환
                    bool val = is_str_true(argv[4]);
                    printf("%s\n", bitmap_contains(bm, start, count, val) ? "true" : "false");
                }

                else if (strcmp(argv[0], "bitmap_count") == 0 && argc == 5)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    // 입력 받은 문자열을 bool형으로 변환
                    bool val = is_str_true(argv[4]);
                    printf("%zu\n", bitmap_count(bm, start, count, val));
                }

                else if (strcmp(argv[0], "bitmap_dump") == 0 && argc == 2)
                {
                    bitmap_dump(bm);
                }
                else if (strcmp(argv[0], "bitmap_expand") == 0 && argc == 3)
                {
                    int size = atoi(argv[2]);
                    struct bitmap *expanded = bitmap_expand(bm, size);
                    if (expanded != NULL)
                    {
                        bitmaps[idx] = expanded;
                    }
                }
                else if (strcmp(argv[0], "bitmap_scan_and_flip") == 0 && argc == 5)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    bool val = is_str_true(argv[4]);
                    size_t res = bitmap_scan_and_flip(bm, start, count, val);
                    printf("%zu\n", res);
                }
                else if (strcmp(argv[0], "bitmap_scan") == 0 && argc == 5)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    bool val = is_str_true(argv[4]);
                    size_t res = bitmap_scan(bm, start, count, val);
                    printf("%zu\n", res);
                }
                else if (strcmp(argv[0], "bitmap_set_multiple") == 0 && argc == 5)
                {
                    int start = atoi(argv[2]);
                    int count = atoi(argv[3]);
                    bool val = is_str_true(argv[4]);
                    bitmap_set_multiple(bm, start, count, val);
                }
            }
        }
    }
    return 0;
}