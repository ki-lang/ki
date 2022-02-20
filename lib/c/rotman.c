
// Rotman aka. Routine Manager

#include <pthread.h>
#include <setjmp.h>

#include "/home/ctx/.ki/cache/project.h"

#define KI_NUMTHREADS 16
#define KI_MAX_TASKS_PER_R 10

pthread_key_t KI_RM;
char KI_RM_LIST_LOCK = 0;

// typedef struct ki__async__Task {
//   void* handler_func;
//   void* func;
//   void* args;
//   jmp_buf* jmpbuf;
//   void* result;
//   unsigned char ready;
//   short _RC;
// } ki__async__Task;

typedef struct ki__async__Task ki__async__Task;

typedef struct RoutineManager {
  ki__async__Task* tasks[KI_MAX_TASKS_PER_R];
  char current_task;
  char tasks_running;
  jmp_buf jmpbuf;
} RoutineManager;

RoutineManager* KI_RM_LIST[KI_NUMTHREADS];
ki__async__Task* KI_RM_TASK_LIST[1024];
ki__async__Task* KI_RM_TASK_LIST_PRIO[1024];
int KI_RM_TASK_LIST_C = 0;
int KI_RM_TASK_LIST_PRIO_C = 0;

void* KI_RM_init_thread(void* i);
void KI_RM_task_run_loop(RoutineManager* rm);

void KI_RM_init() {
  pthread_t threads[KI_NUMTHREADS];

  pthread_key_create(&KI_RM, NULL);
  for (int i = 0; i < KI_NUMTHREADS; i++) {
    pthread_create(threads + i, NULL, KI_RM_init_thread, (void*)(size_t)i);
  }

  // Run main

  // Wait until all threads are done
  int run_count = 0;
  while (1) {
    run_count = 0;
    for (int i = 0; i < KI_NUMTHREADS; i++) {
      RoutineManager* rm = KI_RM_LIST[i];
      if (rm->tasks_running > 0) {
        run_count++;
      }
    }
    if (run_count == 0) {
      break;
    }
  }
}

void* KI_RM_init_thread(void* i) {
  //
  size_t nr = (size_t)i;
  //
  RoutineManager* rm = ki__mem__alloc(sizeof(RoutineManager));
  rm->current_task = 0;
  rm->tasks_running = 0;

  KI_RM_LIST[nr] = rm;

  pthread_setspecific(KI_RM, rm);

  KI_RM_task_run_loop(rm);

  pthread_setspecific(KI_RM, NULL);
  ki__mem__free(rm);
}

void KI_RM_task_run_loop(RoutineManager* rm) {
  //
  while (1) {
    // Run a task
    if (rm->tasks_running > 0) {
      if (rm->tasks[rm->current_task]) {
        if (!setjmp(rm->jmpbuf)) {
          ki__async__Task* task = rm->tasks[rm->current_task];
          if (task->jmpbuf) {
            longjmp(*(jmp_buf*)task->jmpbuf, 1);
          } else {
            ((void (*)(ki__async__Task*))task->handler_func)(task);
            rm->tasks_running--;
          }
        }
      } else {
        rm->current_task++;
        if (rm->current_task == KI_MAX_TASKS_PER_R) {
          rm->current_task = 0;
        }
      }
    }
    //
    if (rm->tasks_running < 2) {
      // Get new task
      while (KI_RM_LIST_LOCK) {
      }
      KI_RM_LIST_LOCK = 1;
      int pos = 0;
      while (1) {
        ki__async__Task* task = KI_RM_TASK_LIST[pos];
        if (task) {
          for (int i = 0; i < KI_MAX_TASKS_PER_R; i++) {
            if (rm->tasks[i] == NULL) {
              rm->tasks[i] = task;
              KI_RM_TASK_LIST[pos] = NULL;
              rm->tasks_running++;
              break;
            }
          }
        }
        pos++;
        if (pos == 1024) {
          break;
        }
      }
      KI_RM_LIST_LOCK = 0;
    }
  }
}

void KI_RM_run_next_task() {
  //
  RoutineManager* rm = pthread_getspecific(KI_RM);
  ki__async__Task* task = rm->tasks[rm->current_task];
  // Jump back to task runner
  if (setjmp(*(jmp_buf*)task->jmpbuf)) {
    rm->current_task++;
    if (rm->current_task == KI_MAX_TASKS_PER_R) {
      rm->current_task = 0;
    }
    longjmp(rm->jmpbuf, 1);
  }
}

// void KI_RM_move_current_to_prio() {
//   // Used when the current thread is 'await'-ing a result
//   // But there is no result yet.
//   //
//   RoutineManager* rm = pthread_getspecific(KI_RM);
//   ki__async__Task* task = rm->tasks[rm->current_task];
//   // Remove current
//   rm->tasks[rm->current_task] = NULL;
//   rm->tasks_running--;
//   // Add to prio
//   KI_RM_TASK_LIST_PRIO_C++;
//   for (int i = 0; i < 1024; i++) {
//     void* x = KI_RM_TASK_LIST_PRIO[i];
//     if (x == NULL) {
//       KI_RM_TASK_LIST_PRIO[i] = task;
//     }
//     if (i == 1024 - 1) {
//       i = 0;
//     }
//   }
//   // Jump back to task runner
//   if (setjmp(task->jmpbuf)) {
//     longjmp(rm->jmpbuf, 1);
//   }
// }