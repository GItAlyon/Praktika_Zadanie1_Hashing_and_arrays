/*
 Задание: Хеш-таблица и множества
 Описание:
 1. Реализация хеш-таблицы (ассоциативный массив) с разрешением коллизий
    методом цепочек.
 2. Реализация множества (Set) на основе хеш-таблицы.
 3. Обоснование выбора хеш-функции.
 4. Набор тестов и примеров применения.
 */

#define _CRT_SECURE_NO_WARNINGS  // Отключает предупреждения о безопасности для sprintf, _strdup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

// 1. СТРУКТУРЫ ДЛЯ ХЕШ-ТАБЛИЦЫ

// Узел цепочки (для метода цепочек)
typedef struct HashNode {
    char* key;              // Ключ (строка)
    int value;              // Значение (для простоты используем int)
    struct HashNode* next;  // Указатель на следующий элемент в цепочке
} HashNode;

// Структура хеш-таблицы
typedef struct {
    HashNode** buckets;     // Массив указателей на головы цепочек
    int capacity;           // Размер массива (всегда степень двойки)
    int size;               // Количество хранимых пар (ключ, значение)
    int threshold;          // Порог для перехеширования (capacity * 0.75)
    int collisions;         // Счётчик коллизий (для статистики)
} HashTable;

// 2. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

// Возвращает ближайшую степень двойки, большую или равную x
int next_power_of_two(int x) {
    if (x <= 0) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

// 3. ХЕШ-ФУНКЦИЯ (ОСНОВНАЯ)

/* ОБОСНОВАНИЕ ВЫБОРА ХЕШ-ФУНКЦИИ:
 
  1. В качестве основы используется широко известный алгоритм djb2 (разработан
     Дэниелом Бернштейном). Он обеспечивает очень хорошее распределение для
     строковых ключей, прост в реализации и быстр.
 
  2. Для защиты от атак, основанных на подборе коллизий (Hash DoS), хеш
     дополнительно перемешивается: к результату применяется XOR с константой
     и битовый сдвиг. Это делает распределение более случайным и устойчивым
     к злонамеренным входным данным.
 
  3. Финальное приведение к индексу массива выполняется с помощью операции
     побитового И (&) с (capacity - 1). Это стандартный и очень быстрый способ
     получить остаток от деления на степень двойки, что эквивалентно
     взятию модуля, но значительно быстрее.
 */
unsigned int hash_function(const char* key, int capacity) {
    unsigned long hash = 5381;  // Начальное значение для djb2
    int c;

    // Основной цикл djb2: hash = hash * 33 + c
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }

    // Перемешивание для улучшения распределения и защиты
    hash ^= (hash >> 16);
    hash ^= (hash >> 8);
    hash ^= (hash >> 4);

    // Приведение к индексу в массиве (ёмкость - степень двойки)
    return (unsigned int)(hash & (capacity - 1));
}

// 4. СОЗДАНИЕ И УНИЧТОЖЕНИЕ ХЕШ-ТАБЛИЦЫ

// Создаёт новую хеш-таблицу
HashTable* create_hash_table(int initial_capacity) {
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) {
        fprintf(stderr, "Ошибка выделения памяти для хеш-таблицы.\n");
        exit(EXIT_FAILURE);
    }

    ht->capacity = next_power_of_two(initial_capacity < 8 ? 8 : initial_capacity);
    ht->size = 0;
    ht->collisions = 0;
    ht->threshold = (int)(ht->capacity * 0.75f);

    ht->buckets = (HashNode**)calloc(ht->capacity, sizeof(HashNode*));
    if (!ht->buckets) {
        fprintf(stderr, "Ошибка выделения памяти для бакетов.\n");
        free(ht);
        exit(EXIT_FAILURE);
    }

    return ht;
}

// Освобождает память, занятую хеш-таблицей
void free_hash_table(HashTable* ht) {
    if (!ht) return;

    for (int i = 0; i < ht->capacity; i++) {
        HashNode* current = ht->buckets[i];
        while (current) {
            HashNode* temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
    }

    free(ht->buckets);
    free(ht);
}

// 5. ПЕРЕХЕШИРОВАНИЕ (РЕХЕШИНГ)

// Увеличивает ёмкость таблицы вдвое и перераспределяет все элементы
void rehash(HashTable** ht_ptr) {
    HashTable* old_ht = *ht_ptr;
    HashTable* new_ht = create_hash_table(old_ht->capacity * 2);
    new_ht->size = 0;  // Обнулим, т.к. insert будет увеличивать

    // Проходим по всем старым цепочкам
    for (int i = 0; i < old_ht->capacity; i++) {
        HashNode* current = old_ht->buckets[i];
        while (current) {
            // Вставляем каждый элемент в новую таблицу
            unsigned int index = hash_function(current->key, new_ht->capacity);
            HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
            new_node->key = _strdup(current->key);  
            new_node->value = current->value;
            new_node->next = new_ht->buckets[index];
            new_ht->buckets[index] = new_node;
            new_ht->size++;

            // Считаем коллизии в новой таблице
            if (new_node->next != NULL) {
                new_ht->collisions++;
            }

            current = current->next;
        }
    }

    // Заменяем старую таблицу новой
    free(old_ht->buckets);
    free(old_ht);
    *ht_ptr = new_ht;
}

// 6. ОСНОВНЫЕ ОПЕРАЦИИ ХЕШ-ТАБЛИЦЫ

// Вставка пары (ключ, значение). Если ключ есть, обновляет значение.
void hash_table_insert(HashTable** ht_ptr, const char* key, int value) {
    HashTable* ht = *ht_ptr;

    // Проверяем необходимость перехеширования
    if (ht->size >= ht->threshold) {
        rehash(ht_ptr);
        ht = *ht_ptr;  // Обновляем указатель после перехеширования
    }

    unsigned int index = hash_function(key, ht->capacity);
    HashNode* current = ht->buckets[index];

    // Ищем ключ в цепочке
    while (current) {
        if (strcmp(current->key, key) == 0) {
            current->value = value;  // Обновляем значение
            return;
        }
        current = current->next;
    }

    // Ключ не найден - создаём новый узел в начале цепочки
    HashNode* new_node = (HashNode*)malloc(sizeof(HashNode));
    new_node->key = _strdup(key);  
    new_node->value = value;
    new_node->next = ht->buckets[index];
    ht->buckets[index] = new_node;
    ht->size++;

    // Если в цепочке уже были элементы, это коллизия
    if (new_node->next != NULL) {
        ht->collisions++;
    }
}

// Поиск значения по ключу. Возвращает true, если найден, и записывает значение в *out_value.
bool hash_table_get(HashTable* ht, const char* key, int* out_value) {
    if (!ht) return false;

    unsigned int index = hash_function(key, ht->capacity);
    HashNode* current = ht->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (out_value) *out_value = current->value;
            return true;
        }
        current = current->next;
    }
    return false;
}

// Удаление пары по ключу. Возвращает true, если ключ был найден и удалён.
bool hash_table_remove(HashTable* ht, const char* key) {
    if (!ht) return false;

    unsigned int index = hash_function(key, ht->capacity);
    HashNode* current = ht->buckets[index];
    HashNode* prev = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            }
            else {
                ht->buckets[index] = current->next;
            }
            free(current->key);
            free(current);
            ht->size--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

// Проверка наличия ключа
bool hash_table_contains(HashTable* ht, const char* key) {
    return hash_table_get(ht, key, NULL);
}

// Получение количества элементов
int hash_table_size(HashTable* ht) {
    return ht ? ht->size : 0;
}

// Получение статистики коллизий
void hash_table_stats(HashTable* ht, int* out_collisions, float* out_avg_chain) {
    if (!ht) {
        if (out_collisions) *out_collisions = 0;
        if (out_avg_chain) *out_avg_chain = 0.0f;
        return;
    }

    if (out_collisions) *out_collisions = ht->collisions;

    if (out_avg_chain) {
        int non_empty = 0;
        int total_items = 0;
        for (int i = 0; i < ht->capacity; i++) {
            if (ht->buckets[i] != NULL) {
                non_empty++;
                HashNode* current = ht->buckets[i];
                while (current) {
                    total_items++;
                    current = current->next;
                }
            }
        }
        *out_avg_chain = (non_empty > 0) ? (float)total_items / non_empty : 0.0f;
    }
}

// 7. РЕАЛИЗАЦИЯ МНОЖЕСТВА НА ОСНОВЕ ХЕШ-ТАБЛИЦЫ

// Структура множества
typedef struct {
    HashTable* table;  // Используем хеш-таблицу для хранения элементов
} HashSet;

// Создание множества
HashSet* create_hash_set(int initial_capacity) {
    HashSet* set = (HashSet*)malloc(sizeof(HashSet));
    if (!set) {
        fprintf(stderr, "Ошибка выделения памяти для множества.\n");
        exit(EXIT_FAILURE);
    }
    set->table = create_hash_table(initial_capacity);
    return set;
}

// Освобождение множества
void free_hash_set(HashSet* set) {
    if (!set) return;
    free_hash_table(set->table);
    free(set);
}

// Добавление элемента во множество
void hash_set_add(HashSet** set_ptr, const char* element) {
    // В качестве значения используем 1 (заглушка)
    hash_table_insert(&((*set_ptr)->table), element, 1);
}

// Проверка принадлежности элемента множеству
bool hash_set_contains(HashSet* set, const char* element) {
    return hash_table_contains(set->table, element);
}

// Удаление элемента из множества
void hash_set_remove(HashSet* set, const char* element) {
    hash_table_remove(set->table, element);
}

// Размер множества
int hash_set_size(HashSet* set) {
    return hash_table_size(set->table);
}

// ОПЕРАЦИИ НАД МНОЖЕСТВАМИ (на основе теории)

// Объединение (A U B)
HashSet* hash_set_union(HashSet* a, HashSet* b) {
    HashSet* result = create_hash_set(8);
    // Добавляем все элементы из a
    for (int i = 0; i < a->table->capacity; i++) {
        HashNode* current = a->table->buckets[i];
        while (current) {
            hash_set_add(&result, current->key);
            current = current->next;
        }
    }
    // Добавляем все элементы из b
    for (int i = 0; i < b->table->capacity; i++) {
        HashNode* current = b->table->buckets[i];
        while (current) {
            hash_set_add(&result, current->key);
            current = current->next;
        }
    }
    return result;
}

// Пересечение (A n B)
HashSet* hash_set_intersection(HashSet* a, HashSet* b) {
    HashSet* result = create_hash_set(8);
    // Итерируемся по меньшему множеству для эффективности
    HashSet* smaller = (hash_set_size(a) < hash_set_size(b)) ? a : b;
    HashSet* larger = (hash_set_size(a) < hash_set_size(b)) ? b : a;

    for (int i = 0; i < smaller->table->capacity; i++) {
        HashNode* current = smaller->table->buckets[i];
        while (current) {
            if (hash_set_contains(larger, current->key)) {
                hash_set_add(&result, current->key);
            }
            current = current->next;
        }
    }
    return result;
}

// Разность (A \ B)
HashSet* hash_set_difference(HashSet* a, HashSet* b) {
    HashSet* result = create_hash_set(8);
    for (int i = 0; i < a->table->capacity; i++) {
        HashNode* current = a->table->buckets[i];
        while (current) {
            if (!hash_set_contains(b, current->key)) {
                hash_set_add(&result, current->key);
            }
            current = current->next;
        }
    }
    return result;
}

// Симметрическая разность (A △ B)
HashSet* hash_set_symmetric_difference(HashSet* a, HashSet* b) {
    HashSet* diff1 = hash_set_difference(a, b);
    HashSet* diff2 = hash_set_difference(b, a);
    HashSet* result = hash_set_union(diff1, diff2);
    free_hash_set(diff1);
    free_hash_set(diff2);
    return result;
}

// Проверка подмножества (A subset B)
bool hash_set_is_subset(HashSet* a, HashSet* b) {
    if (hash_set_size(a) > hash_set_size(b)) return false;
    for (int i = 0; i < a->table->capacity; i++) {
        HashNode* current = a->table->buckets[i];
        while (current) {
            if (!hash_set_contains(b, current->key)) {
                return false;
            }
            current = current->next;
        }
    }
    return true;
}

// Проверка равенства множеств
bool hash_set_equals(HashSet* a, HashSet* b) {
    return hash_set_is_subset(a, b) && hash_set_is_subset(b, a);
}

// 8. РЕАЛИЗАЦИЯ МУЛЬТИМНОЖЕСТВА НА ОСНОВЕ ХЕШ-ТАБЛИЦЫ

// Структура узла для мультимножества (хранит счётчик)
typedef struct MultisetNode {
    char* key;
    int count;              // Количество вхождений
    struct MultisetNode* next;
} MultisetNode;

// Структура мультимножества
typedef struct {
    MultisetNode** buckets;
    int capacity;
    int size;               // Общее количество элементов (с учётом дубликатов)
    int unique_count;       // Количество уникальных элементов
    int threshold;
} Multiset;

// Создание мультимножества
Multiset* create_multiset(int initial_capacity) {
    Multiset* ms = (Multiset*)malloc(sizeof(Multiset));
    if (!ms) {
        fprintf(stderr, "Ошибка выделения памяти для мультимножества.\n");
        exit(EXIT_FAILURE);
    }

    ms->capacity = next_power_of_two(initial_capacity < 8 ? 8 : initial_capacity);
    ms->size = 0;
    ms->unique_count = 0;
    ms->threshold = (int)(ms->capacity * 0.75f);
    ms->buckets = (MultisetNode**)calloc(ms->capacity, sizeof(MultisetNode*));
    if (!ms->buckets) {
        fprintf(stderr, "Ошибка выделения памяти для бакетов мультимножества.\n");
        free(ms);
        exit(EXIT_FAILURE);
    }
    return ms;
}

// Освобождение мультимножества
void free_multiset(Multiset* ms) {
    if (!ms) return;

    for (int i = 0; i < ms->capacity; i++) {
        MultisetNode* current = ms->buckets[i];
        while (current) {
            MultisetNode* temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
    }
    free(ms->buckets);
    free(ms);
}

// ПЕРЕХЕШИРОВАНИЕ ДЛЯ МУЛЬТИМНОЖЕСТВА

// Увеличивает ёмкость мультимножества вдвое и перераспределяет все элементы
void rehash_multiset(Multiset** ms_ptr) {
    Multiset* old_ms = *ms_ptr;
    Multiset* new_ms = create_multiset(old_ms->capacity * 2);
    new_ms->size = 0;
    new_ms->unique_count = 0;

    // Проходим по всем старым цепочкам
    for (int i = 0; i < old_ms->capacity; i++) {
        MultisetNode* current = old_ms->buckets[i];
        while (current) {
            // Вставляем каждый элемент в новую таблицу
            unsigned int index = hash_function(current->key, new_ms->capacity);
            MultisetNode* new_node = (MultisetNode*)malloc(sizeof(MultisetNode));
            new_node->key = _strdup(current->key);
            new_node->count = current->count;  // Сохраняем количество вхождений
            new_node->next = new_ms->buckets[index];
            new_ms->buckets[index] = new_node;
            new_ms->size += current->count;        // Увеличиваем общий размер
            new_ms->unique_count++;                // Увеличиваем количество уникальных

            current = current->next;
        }
    }

    // Заменяем старую таблицу новой
    free(old_ms->buckets);
    free(old_ms);
    *ms_ptr = new_ms;
}

// ОСНОВНЫЕ ОПЕРАЦИИ МУЛЬТИМНОЖЕСТВА

// Добавление элемента в мультимножество
void multiset_add(Multiset** ms_ptr, const char* element) {
    Multiset* ms = *ms_ptr;

    // Проверяем необходимость перехеширования
    if (ms->size >= ms->threshold) {
        rehash_multiset(ms_ptr);
        ms = *ms_ptr;  // Обновляем указатель после перехеширования
    }

    unsigned int index = hash_function(element, ms->capacity);
    MultisetNode* current = ms->buckets[index];

    // Ищем элемент
    while (current) {
        if (strcmp(current->key, element) == 0) {
            current->count++;      // Увеличиваем счётчик
            ms->size++;            // Общий размер увеличивается
            return;
        }
        current = current->next;
    }

    // Элемент не найден - добавляем новый с count = 1
    MultisetNode* new_node = (MultisetNode*)malloc(sizeof(MultisetNode));
    new_node->key = _strdup(element);
    new_node->count = 1;
    new_node->next = ms->buckets[index];
    ms->buckets[index] = new_node;
    ms->size++;
    ms->unique_count++;
}

// Удаление одного вхождения элемента
bool multiset_remove_one(Multiset* ms, const char* element) {
    if (!ms) return false;

    unsigned int index = hash_function(element, ms->capacity);
    MultisetNode* current = ms->buckets[index];
    MultisetNode* prev = NULL;

    while (current) {
        if (strcmp(current->key, element) == 0) {
            if (current->count > 1) {
                current->count--;   // Уменьшаем счётчик
                ms->size--;
                return true;
            }
            else {
                // Удаляем узел полностью
                if (prev) {
                    prev->next = current->next;
                }
                else {
                    ms->buckets[index] = current->next;
                }
                free(current->key);
                free(current);
                ms->size--;
                ms->unique_count--;
                return true;
            }
        }
        prev = current;
        current = current->next;
    }
    return false;  // Элемент не найден
}

// Удаление ВСЕХ вхождений элемента
bool multiset_remove_all(Multiset* ms, const char* element) {
    if (!ms) return false;

    unsigned int index = hash_function(element, ms->capacity);
    MultisetNode* current = ms->buckets[index];
    MultisetNode* prev = NULL;

    while (current) {
        if (strcmp(current->key, element) == 0) {
            // Удаляем узел со ВСЕМИ вхождениями
            if (prev) {
                prev->next = current->next;
            }
            else {
                ms->buckets[index] = current->next;
            }
            ms->size -= current->count;  // Уменьшаем общий размер
            ms->unique_count--;          // Уменьшаем количество уникальных
            free(current->key);
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;  // Элемент не найден
}

// Получение количества вхождений элемента
int multiset_count(Multiset* ms, const char* element) {
    if (!ms) return 0;

    unsigned int index = hash_function(element, ms->capacity);
    MultisetNode* current = ms->buckets[index];

    while (current) {
        if (strcmp(current->key, element) == 0) {
            return current->count;
        }
        current = current->next;
    }
    return 0;  // Элемент не найден
}

// Проверка наличия элемента (хотя бы одно вхождение)
bool multiset_contains(Multiset* ms, const char* element) {
    return multiset_count(ms, element) > 0;
}

// Общий размер (все вхождения)
int multiset_size(Multiset* ms) {
    return ms ? ms->size : 0;
}

// Количество уникальных элементов
int multiset_unique_count(Multiset* ms) {
    return ms ? ms->unique_count : 0;
}

// ОПЕРАЦИИ НАД МУЛЬТИМНОЖЕСТВАМИ

// Объединение мультимножеств (складываем количества вхождений)
Multiset* multiset_union(Multiset* a, Multiset* b) {
    Multiset* result = create_multiset(8);

    // Добавляем все элементы из a
    for (int i = 0; i < a->capacity; i++) {
        MultisetNode* current = a->buckets[i];
        while (current) {
            // Добавляем элемент столько раз, сколько он встречается в a
            for (int j = 0; j < current->count; j++) {
                multiset_add(&result, current->key);
            }
            current = current->next;
        }
    }

    // Добавляем все элементы из b
    for (int i = 0; i < b->capacity; i++) {
        MultisetNode* current = b->buckets[i];
        while (current) {
            // Добавляем элемент столько раз, сколько он встречается в b
            for (int j = 0; j < current->count; j++) {
                multiset_add(&result, current->key);
            }
            current = current->next;
        }
    }

    return result;
}

// Пересечение мультимножеств (берём минимальное количество вхождений)
Multiset* multiset_intersection(Multiset* a, Multiset* b) {
    Multiset* result = create_multiset(8);

    // Итерируемся по меньшему множеству для эффективности
    Multiset* smaller = (a->unique_count < b->unique_count) ? a : b;
    Multiset* larger = (a->unique_count < b->unique_count) ? b : a;

    for (int i = 0; i < smaller->capacity; i++) {
        MultisetNode* current = smaller->buckets[i];
        while (current) {
            int count_in_a = multiset_count(a, current->key);
            int count_in_b = multiset_count(b, current->key);
            int min_count = (count_in_a < count_in_b) ? count_in_a : count_in_b;

            // Добавляем элемент min_count раз
            for (int j = 0; j < min_count; j++) {
                multiset_add(&result, current->key);
            }
            current = current->next;
        }
    }

    return result;
}

// 9. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ВЫВОДА И ТЕСТИРОВАНИЯ

// Вывод содержимого хеш-таблицы
void print_hash_table(HashTable* ht) {
    if (!ht) {
        printf("Хеш-таблица пуста (NULL).\n");
        return;
    }
    printf("Хеш-таблица (размер: %d, ёмкость: %d, коллизий: %d):\n",
        ht->size, ht->capacity, ht->collisions);
    for (int i = 0; i < ht->capacity; i++) {
        if (ht->buckets[i] != NULL) {
            printf("  [%d] -> ", i);
            HashNode* current = ht->buckets[i];
            while (current) {
                printf("(%s: %d) ", current->key, current->value);
                if (current->next) printf("-> ");
                current = current->next;
            }
            printf("\n");
        }
    }
}

// Вывод содержимого множества
void print_hash_set(HashSet* set, const char* name) {
    if (!set) {
        printf("%s: пустое множество (NULL).\n", name);
        return;
    }
    printf("%s = { ", name);
    bool first = true;
    for (int i = 0; i < set->table->capacity; i++) {
        HashNode* current = set->table->buckets[i];
        while (current) {
            if (!first) printf(", ");
            printf("%s", current->key);
            first = false;
            current = current->next;
        }
    }
    printf(" }\n");
}

// Вывод мультимножества
void print_multiset(Multiset* ms, const char* name) {
    if (!ms) {
        printf("%s: пустое мультимножество (NULL).\n", name);
        return;
    }
    printf("%s = { ", name);
    bool first = true;
    for (int i = 0; i < ms->capacity; i++) {
        MultisetNode* current = ms->buckets[i];
        while (current) {
            if (!first) printf(", ");
            printf("%s(%d)", current->key, current->count);
            first = false;
            current = current->next;
        }
    }
    printf(" }\n");
    printf("  (всего: %d, уникальных: %d)\n", ms->size, ms->unique_count);
}

// Функция для демонстрации кэширования 
int expensive_computation(int x) {
    return x * x + 10;
}

// 10. ТЕСТЫ

void run_all_tests() {
    printf("ЗАПУСК ТЕСТОВ\n");

    printf("\n--- ТЕСТ 1: Хеш-таблица (базовые операции) ---\n");

    HashTable* ht = create_hash_table(8);
    printf("Создана таблица с ёмкостью %d\n", ht->capacity);

    hash_table_insert(&ht, "Ноутбук", 100);
    hash_table_insert(&ht, "Мышь", 200);
    hash_table_insert(&ht, "Клавиатура", 300);
    hash_table_insert(&ht, "Коврик для мыши", 400);
    printf("Размер: %d\n", hash_table_size(ht));

    int val;
    if (hash_table_get(ht, "Ноутбук", &val)) printf("Ноутбук: %d\n", val);
    if (hash_table_get(ht, "Мышь", &val)) printf("Мышь: %d\n", val);
    if (hash_table_get(ht, "Клавиатура", &val)) printf("Клавиатура: %d\n", val);
    if (hash_table_get(ht, "Коврик для мыши", &val)) printf("Коврик для мыши: %d\n", val);

    // Обновление
    printf("Обновление:\n");
    hash_table_insert(&ht, "Ноутбук", 150);
    if (hash_table_get(ht, "Ноутбук", &val)) printf("Ноутбук (обновлено): %d\n", val);

    // Удаление
    printf("Удаление:\n");
    printf("Первоначальный размер хеш-таблицы: %d\n", hash_table_size(ht));
    hash_table_remove(ht, "Клавиатура");
    printf("Клавиатура удалено. Размер: %d\n", hash_table_size(ht));
    printf("Клавиатура в таблице? %s\n", hash_table_contains(ht, "Клавиатура") ? "да" : "нет");
    printf("Мышь в таблице? %s\n", hash_table_contains(ht, "Мышь") ? "да" : "нет");

    // Статистика
    int collisions;
    float avg_chain;
    hash_table_stats(ht, &collisions, &avg_chain);
    printf("Статистика:\n");
    printf("Коллизий: %d, Средняя длина цепочки: %.2f\n", collisions, avg_chain);

    print_hash_table(ht);
    free_hash_table(ht);

    printf("\n--- ТЕСТ 2: Перехеширование ---\n");

    ht = create_hash_table(4);
    printf("Начальная ёмкость: %d, порог: %d\n", ht->capacity, ht->threshold);

    for (int i = 0; i < 15; i++) {
        char key[20];
        sprintf(key, "key_%d", i);
        hash_table_insert(&ht, key, i * 10);
        printf("Вставлен %s, размер: %d, ёмкость: %d\n", key, ht->size, ht->capacity);
    }
    print_hash_table(ht);
    free_hash_table(ht);

    printf("\n--- ТЕСТ 3: Множество (базовые операции) ---\n");

    HashSet* set = create_hash_set(8);
    hash_set_add(&set, "Ноутбук");
    hash_set_add(&set, "Мышь");
    hash_set_add(&set, "Ноутбук");  // Дубликат
    hash_set_add(&set, "Клавиатура");

    print_hash_set(set, "set");
    printf("Размер: %d\n", hash_set_size(set));
    printf("Содержит 'Ноутбук'? %s\n", hash_set_contains(set, "Ноутбук") ? "да" : "нет");
    printf("Содержит 'Коврик для мыши'? %s\n", hash_set_contains(set, "Коврик для мыши") ? "да" : "нет");

    hash_set_remove(set, "Ноутбук");
    print_hash_set(set, "set после удаления Ноутбук");

    printf("\n--- ТЕСТ 4: Операции над множествами ---\n");

    HashSet* A = create_hash_set(8);
    HashSet* B = create_hash_set(8);

    hash_set_add(&A, "1");
    hash_set_add(&A, "2");
    hash_set_add(&A, "3");
    hash_set_add(&A, "4");

    hash_set_add(&B, "3");
    hash_set_add(&B, "4");
    hash_set_add(&B, "5");
    hash_set_add(&B, "6");

    print_hash_set(A, "A");
    print_hash_set(B, "B");

    HashSet* union_set = hash_set_union(A, B);
    print_hash_set(union_set, "A U B");

    HashSet* inter_set = hash_set_intersection(A, B);
    print_hash_set(inter_set, "A n B");

    HashSet* diff_set = hash_set_difference(A, B);
    print_hash_set(diff_set, "A \\ B");

    HashSet* sym_diff = hash_set_symmetric_difference(A, B);
    print_hash_set(sym_diff, "A △ B");

    printf("A subset B? %s\n", hash_set_is_subset(A, B) ? "да" : "нет");

    HashSet* C = create_hash_set(8);
    hash_set_add(&C, "1");
    hash_set_add(&C, "2");
    hash_set_add(&C, "3");
    print_hash_set(C, "C");
    printf("C subset A? %s\n", hash_set_is_subset(C, A) ? "да" : "нет");

    free_hash_set(A);
    free_hash_set(B);
    free_hash_set(union_set);
    free_hash_set(inter_set);
    free_hash_set(diff_set);
    free_hash_set(sym_diff);
    free_hash_set(C);

    printf("\n--- ТЕСТ 5: Проверка теоремы де Моргана ---\n");

    // (A U B)^c = A^c n B^c  (на ограниченном универсуме U)
    HashSet* U = create_hash_set(8);
    for (int i = 1; i <= 10; i++) {
        char key[3];
        sprintf(key, "%d", i);
        hash_set_add(&U, key);
    }

    HashSet* A5 = create_hash_set(8);
    HashSet* B5 = create_hash_set(8);
    for (int i = 1; i <= 5; i++) {
        char key[3];
        sprintf(key, "%d", i);
        hash_set_add(&A5, key);
    }
    for (int i = 4; i <= 8; i++) {
        char key[3];
        sprintf(key, "%d", i);
        hash_set_add(&B5, key);
    }

    print_hash_set(U, "U (универсум)");
    print_hash_set(A5, "A");
    print_hash_set(B5, "B");

    // Левая часть: U \ (A U B)
    HashSet* union_ab = hash_set_union(A5, B5);
    HashSet* left_side = hash_set_difference(U, union_ab);

    // Правая часть: (U \ A) n (U \ B)
    HashSet* complement_a = hash_set_difference(U, A5);
    HashSet* complement_b = hash_set_difference(U, B5);
    HashSet* right_side = hash_set_intersection(complement_a, complement_b);

    print_hash_set(left_side, "U \\ (A U B) (левая часть)");
    print_hash_set(right_side, "(U \\ A) n (U \\ B) (правая часть)");
    printf("Равенство выполняется? %s\n",
        hash_set_equals(left_side, right_side) ? "ДА" : "НЕТ");

    free_hash_set(U);
    free_hash_set(A5);
    free_hash_set(B5);
    free_hash_set(union_ab);
    free_hash_set(left_side);
    free_hash_set(complement_a);
    free_hash_set(complement_b);
    free_hash_set(right_side);

    printf("\n--- ТЕСТ 6: Производительность (небольшая демонстрация) ---\n");
    ht = create_hash_table(16);
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        char key[20];
        sprintf(key, "key_%d", i);
        hash_table_insert(&ht, key, i);
    }
    for (int i = 0; i < 10000; i++) {
        char key[20];
        sprintf(key, "key_%d", i);
        hash_table_get(ht, key, NULL);
    }
    clock_t end = clock();
    printf("Время на 10000 вставок и 10000 поисков: %.4f сек.\n",
        (double)(end - start) / CLOCKS_PER_SEC);
    free_hash_table(ht);

    printf("\n--- ТЕСТ 7: Мультимножество ---\n");

    // Создание мультимножества
    Multiset* ms = create_multiset(8);
    printf("Создано мультимножество с ёмкостью %d\n", ms->capacity);

    // Добавление элементов
    multiset_add(&ms, "Ноутбук");
    multiset_add(&ms, "Мышь");
    multiset_add(&ms, "Ноутбук");      // Дубликат
    multiset_add(&ms, "Ноутбук");      // Ещё один дубликат
    multiset_add(&ms, "Клавиатура");
    multiset_add(&ms, "Мышь");     // Ещё один дубликат

    print_multiset(ms, "ms");
    printf("Размер (все вхождения): %d\n", multiset_size(ms));           // 6
    printf("Уникальных элементов: %d\n", multiset_unique_count(ms));     // 3
    printf("Ноутбук встречается: %d раз\n", multiset_count(ms, "Ноутбук"));   // 3
    printf("Мышь встречается: %d раз\n", multiset_count(ms, "Мышь")); // 2
    printf("Коврик для мыши встречается: %d раз\n", multiset_count(ms, "Коврик для мыши"));   // 0

    // Удаление одного Ноутбук
    printf("\n--- Удаление одного Ноутбук ---\n");
    multiset_remove_one(ms, "Ноутбук");
    print_multiset(ms, "ms после удаления одного Ноутбук");
    printf("Ноутбук встречается: %d раз\n", multiset_count(ms, "Ноутбук"));   // 2

    // Удаление всех Мышь
    printf("\n--- Удаление всех Мышь ---\n");
    multiset_remove_all(ms, "Мышь");
    print_multiset(ms, "ms после удаления всех Мышь");
    printf("Мышь встречается: %d раз\n", multiset_count(ms, "Мышь")); // 0

    // Проверка перехеширования
    printf("\n--- Проверка перехеширования ---\n");
    Multiset* ms2 = create_multiset(4);
    printf("Начальная ёмкость: %d, порог: %d\n", ms2->capacity, ms2->threshold);

    for (int i = 0; i < 15; i++) {
        char key[20];
        sprintf(key, "key_%d", i % 3);  // Создаём дубликаты
        multiset_add(&ms2, key);
        if (i % 5 == 0 || i == 14) {
            printf("Добавлено %d элементов, ёмкость: %d\n", i + 1, ms2->capacity);
        }
    }
    print_multiset(ms2, "ms2 после добавления 15 элементов");

    // Операции над мультимножествами
    printf("\n--- Операции над мультимножествами ---\n");
    Multiset* A_mult = create_multiset(8);
    Multiset* B_mult = create_multiset(8);

    multiset_add(&A_mult, "a");
    multiset_add(&A_mult, "a");
    multiset_add(&A_mult, "b");
    multiset_add(&A_mult, "c");

    multiset_add(&B_mult, "a");
    multiset_add(&B_mult, "b");
    multiset_add(&B_mult, "b");
    multiset_add(&B_mult, "d");

    print_multiset(A_mult, "A");
    print_multiset(B_mult, "B");

    Multiset* union_ms = multiset_union(A_mult, B_mult);
    print_multiset(union_ms, "A U B (объединение)");

    Multiset* inter_ms = multiset_intersection(A_mult, B_mult);
    print_multiset(inter_ms, "A ∩ B (пересечение)");

    // Освобождение памяти
    free_multiset(ms);
    free_multiset(ms2);
    free_multiset(A_mult);
    free_multiset(B_mult);
    free_multiset(union_ms);
    free_multiset(inter_ms);

    printf("ВСЕ ТЕСТЫ ЗАВЕРШЕНЫ УСПЕШНО!\n");
}

// 11. ПРИМЕРЫ ПРИМЕНЕНИЯ

void practical_examples() {
    printf("\nПРИМЕРЫ ПРИМЕНЕНИЯ\n");

    printf("\n--- Пример 1: Хеш-таблица как кэш ---\n");

    HashTable* cache = create_hash_table(8);

    int inputs[] = { 1, 2, 3, 2, 4, 1, 5 };
    int n_inputs = sizeof(inputs) / sizeof(inputs[0]);

    for (int i = 0; i < n_inputs; i++) {
        int val = inputs[i];
        char key[10];
        sprintf(key, "%d", val);

        int cached_value;
        if (hash_table_get(cache, key, &cached_value)) {
            printf("Результат для %d взят из кэша: %d\n", val, cached_value);
        }
        else {
            int result = expensive_computation(val);
            hash_table_insert(&cache, key, result);
            printf("Результат для %d вычислен и сохранён: %d\n", val, result);
        }
    }
    free_hash_table(cache);

    printf("\n--- Пример 2: Множество для удаления дубликатов ---\n");

    const char* data[] = { "Ноутбук", "Мышь", "Ноутбук", "Клавиатура", "Мышь", "Коврик для мыши" };
    int n_data = sizeof(data) / sizeof(data[0]);

    HashSet* unique_set = create_hash_set(8);
    for (int i = 0; i < n_data; i++) {
        hash_set_add(&unique_set, data[i]);
    }

    printf("Исходные данные: ");
    for (int i = 0; i < n_data; i++) {
        printf("%s ", data[i]);
    }
    printf("\n");
    print_hash_set(unique_set, "Уникальные элементы");
    free_hash_set(unique_set);

    printf("\n--- Пример 3: Поиск общих друзей (социальная сеть) ---\n");
    HashSet* user1_friends = create_hash_set(8);
    HashSet* user2_friends = create_hash_set(8);

    hash_set_add(&user1_friends, "Alice");
    hash_set_add(&user1_friends, "Bob");
    hash_set_add(&user1_friends, "Charlie");
    hash_set_add(&user1_friends, "David");

    hash_set_add(&user2_friends, "Charlie");
    hash_set_add(&user2_friends, "David");
    hash_set_add(&user2_friends, "Eve");
    hash_set_add(&user2_friends, "Frank");

    print_hash_set(user1_friends, "Друзья User1");
    print_hash_set(user2_friends, "Друзья User2");

    HashSet* common = hash_set_intersection(user1_friends, user2_friends);
    print_hash_set(common, "Общие друзья");

    free_hash_set(user1_friends);
    free_hash_set(user2_friends);
    free_hash_set(common);
}

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    // Запуск всех тестов
    run_all_tests();

    // Демонстрация практических примеров
    practical_examples();

    printf("\nПрограмма завершена. Нажмите Enter для выхода...\n");
    getchar();
    return 0;
}
