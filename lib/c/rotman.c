
// Rotman aka. Routine Manager

#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "/home/ctxz/.ki/cache/project.h"

#define KI_NUMTHREADS 8
#define KI_MAX_TASKS_PER_R 10
#define KI_MAX_TASKS 48000

pthread_key_t KI_RM;
pthread_mutex_t KI_RM_LIST_LOCK;
pthread_mutex_t KI_RM_LIST_LOCK_ADD;
pthread_t KI_RM_THREADS[KI_NUMTHREADS];
pthread_cond_t KI_RM_THREAD_CONDS[KI_NUMTHREADS];
pthread_mutex_t KI_RM_THREAD_LOCKS[KI_NUMTHREADS];

typedef struct ki__async__Task ki__async__Task;

typedef struct RoutineManager {
  int nr;
  pthread_t thread;
  char suspended;
  ki__async__Task* tasks[KI_MAX_TASKS_PER_R];
  char current_task;
  char tasks_running;
  jmp_buf jmpbuf;
} RoutineManager;

RoutineManager* KI_RM_LIST[KI_NUMTHREADS];
ki__async__Task* KI_RM_TASK_LIST[KI_MAX_TASKS];
ki__async__Task* KI_RM_TASK_LIST_PRIO[KI_MAX_TASKS];
int KI_RM_TASK_LIST_C = 0;
int KI_RM_TASK_LIST_PRIO_C = 0;

void* KI_RM_init_thread(void* i);
void KI_RM_task_run_loop(RoutineManager* rm);

int main() {
  //
  KI_INITS();

  // Create threads
  pthread_key_create(&KI_RM, NULL);
  for (int i = 0; i < KI_NUMTHREADS; i++) {
    //
    RoutineManager* rm = ki__mem__alloc(sizeof(RoutineManager));
    rm->nr = i;
    rm->current_task = 0;
    rm->tasks_running = 0;

    KI_RM_LIST[i] = rm;
    // printf("set %d | %p | %p\n", i, KI_RM_LIST, KI_RM_LIST[i]);

    pthread_create(&KI_RM_THREADS[i], NULL, KI_RM_init_thread, rm);
  }

  // Run main
  ki_async_main();

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

void* KI_RM_init_thread(void* p) {
  //
  RoutineManager* rm = p;
  rm->thread = KI_RM_THREADS[rm->nr];
  //
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
          int nr = rm->current_task;
          // printf("run-task:%d\n", rm->nr);
          ki__async__Task* task = rm->tasks[nr];
          if (task->jmpbuf) {
            // printf("jmp-task:%d\n", rm->nr);
            longjmp(*(jmp_buf*)task->jmpbuf, 1);
          } else {
            ((void (*)(ki__async__Task*))task->handler_func)(task);
            rm->tasks_running--;
            rm->tasks[nr] = NULL;
            if (task->jmpbuf) ki__mem__free(task->jmpbuf);
            ki__mem__free(task->args);
            if (--task->_RC == 0) ki__async__Task____free(task);
          }
        }
      } else {
        rm->current_task++;
        if (rm->current_task >= KI_MAX_TASKS_PER_R) {
          rm->current_task = 0;
        }
      }
    } else {
      // printf("find-task:%d\n", rm->nr);
      // Get new task
      // pthread_mutex_lock(&KI_RM_LIST_LOCK);
      //   printf("lock\n");

      int pos = 0;
      char found = 0;
      for (pos = rm->nr; pos < KI_MAX_TASKS; pos += KI_NUMTHREADS) {
        ki__async__Task* task = KI_RM_TASK_LIST[pos];
        if (task) {
          for (int i = 0; i < KI_MAX_TASKS_PER_R; i++) {
            if (i > 0) {
              printf("something wrong:%d\n", i);
            }
            if (rm->tasks[i] == NULL) {
              rm->tasks[i] = task;
              rm->tasks_running++;
              KI_RM_TASK_LIST[pos] = NULL;
              found = 1;
              // printf("clear_task: %d\n", pos);
              break;
            }
          }
          if (found) {
            break;
          }
        }
      }

      //   printf("unlock\n");
      // pthread_mutex_unlock(&KI_RM_LIST_LOCK);

      if (!found) {
        // Suspend thread, nothing todo
        rm->suspended = 1;
        // printf("sus:%d\n", rm->nr);
        if (pthread_cond_wait(&KI_RM_THREAD_CONDS[rm->nr],
                              &KI_RM_THREAD_LOCKS[rm->nr]) != 0) {
          perror("pthread_cond_timedwait() error");
          exit(7);
        }
        usleep(1);
      }
    }

    // usleep(10);
  }
}

int KI_RM_push_stack_pos = 0;
int KI_RM_push_thread_pos = 0;

void KI_RM_push_task(ki__async__Task* task) {
  //
  // pthread_mutex_lock(&KI_RM_LIST_LOCK_ADD);

  int pos = 0;
  ki__async__Task* t;
  while (1) {
    t = KI_RM_TASK_LIST[KI_RM_push_stack_pos];
    KI_RM_push_stack_pos++;
    if (KI_RM_push_stack_pos >= KI_MAX_TASKS) {
      KI_RM_push_stack_pos = 0;
    }
    if (t == NULL) {
      break;
    }
  }

  // printf("set_task: %d\n", KI_RM_push_stack_pos);
  KI_RM_TASK_LIST[KI_RM_push_stack_pos] = task;
  int i = KI_RM_push_stack_pos % KI_NUMTHREADS;
  // if (KI_RM_LIST[i]->suspended) {
  KI_RM_LIST[i]->suspended = 0;
  if (pthread_cond_signal(&KI_RM_THREAD_CONDS[i]) != 0) {
    perror("pthread_cond_signal() error");
    exit(4);
  }
  // }

  // pthread_mutex_unlock(&KI_RM_LIST_LOCK_ADD);
}

void KI_RM_run_next_task() {
  //
  RoutineManager* rm = pthread_getspecific(KI_RM);
  ki__async__Task* task = rm->tasks[rm->current_task];
  // Jump back to task runner
  if (!task->jmpbuf) {
    task->jmpbuf = ki__mem__alloc(200);
  }
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