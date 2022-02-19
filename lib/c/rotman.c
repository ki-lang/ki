
// Rotman aka. Routine Manager

#include <pthread.h>
#include <setjmp.h>

#define KI_NUMTHREADS 16
#define KI_MAX_TASKS_PER_R 10

pthread_key_t KI_RM;

typedef struct ki__async__Task {
  void* handler_func;
  void* func;
  void* args;
  jmp_buf* jmpbuf;
  void* result;
  unsigned char ready;
  short _RC;
} ki__async__Task;

typedef struct RoutineManager {
  char paused;
  ki__async__Task* tasks[KI_MAX_TASKS_PER_R];
  char current_task;
  char tasks_running;
  jmp_buf* jmpbuf;
} RoutineManager;

RoutineManager* KI_RM_LIST[KI_NUMTHREADS];
ki__async__Task* KI_RM_TASK_LIST[1024];
ki__async__Task* KI_RM_TASK_LIST_PRIO[1024];
int KI_RM_TASK_LIST_C = 0;
int KI_RM_TASK_LIST_PRIO_C = 0;

void KI_RM_init() {
  pthread_t threads[KI_NUMTHREADS];

  pthread_key_create(&KI_RM, NULL);
  for (int i = 0; i < KI_NUMTHREADS; i++) {
    pthread_create(threads + i, NULL, KI_RM_init_thread, i);
  }

  // Run main

  // Wait until all threads are done
  int run_count = 0;
  while (1) {
    run_count = 0;
    for (int i = 0; i < KI_NUMTHREADS; i++) {
      RoutineManager* rm = KI_RM_LIST[i];
      if (!rm->paused) {
        run_count++;
      }
    }
    if (run_count == 0) {
      break;
    }
  }
}

void KI_RM_init_thread(void* i) {
  //
  int nr = (int)i;
  free(i);
  //
  RoutineManager* rm = malloc(sizeof(RoutineManager));

  KI_RM_LIST[nr] = rm;

  pthread_setspecific(KI_RM, rm);
  rm->paused = 1;

  KI_RM_task_run_loop(rm);

  pthread_setspecific(KI_RM, NULL);
  free(rm);
}

void KI_RM_task_run_loop(RoutineManager* rm) {
  //
  while (1) {
    if (rm->paused) continue;
    // Run a task
    for (int i = 0; i < KI_MAX_TASKS_PER_R; i++) {
    }
  }
}

void KI_RM_run_next_task() {
  //
  RoutineManager* rm = pthread_getspecific(KI_RM);
  ki__async__Task* task = rm->tasks[rm->current_task];
  // Jump back to task runner
  if (setjmp(task->jmpbuf)) {
    longjmp(rm->jmpbuf, 1);
  }
}

void KI_RM_move_current_to_prio() {
  // Used when the current thread is 'await'-ing a result
  // But there is no result yet.
  //
  RoutineManager* rm = pthread_getspecific(KI_RM);
  ki__async__Task* task = rm->tasks[rm->current_task];
  // Remove current
  rm->tasks[rm->current_task] = NULL;
  rm->tasks_running--;
  // Add to prio
  KI_RM_TASK_LIST_PRIO_C++;
  for (int i = 0; i < 1024; i++) {
    void* x = KI_RM_TASK_LIST_PRIO[i];
    if (x == NULL) {
      KI_RM_TASK_LIST_PRIO[i] = task;
    }
    if (i == 1024 - 1) {
      i = 0;
    }
  }
  // Jump back to task runner
  if (setjmp(task->jmpbuf)) {
    longjmp(rm->jmpbuf, 1);
  }
}