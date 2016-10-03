#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"

#define USER_VADDR_BOTTOM ((void *) 0x08048000)
typedef int pid_t;

// /* file descriptor */
// struct file_descriptor
// {
//   /* the unique file descriptor number returns to user process. */
//   int fd_num;
//   /* the owner thread’s thread id of the open file */
//   tid_t owner;
//   /* file that is opened */
//   struct file *file_struct;
//   struct list_elem elem;
// };

// /* a list of open files, represents all the files open by the user process
//  *through syscalls. 
//  */
// struct list open_files;

// /* the lock used by syscalls involving file system to ensure only one thread at
//  * a time is accessing file system 
//  */
struct lock filesys_lock;

static void syscall_handler (struct intr_frame *f);
void check_valid_ptr(const void* vaddr);
void check_valid_buffer(void* buffer, unsigned size);
static void exit(int status);
static void halt(void);
static pid_t exec(const char* cmd_line);
static int wait(pid_t pid);
static bool create(const char * file, unsigned initial_size);
static bool remove(const char* file);
static int filesize(int fd);
static void seek(int fd, unsigned position);
static unsigned tell(int fd);
static int open(const char* file);
static void close(int fd);
static int write(int fd, const void *buffer,unsigned size);
static int read(int fd,void *buffer,unsigned size);
int user_to_kernel_ptr(const void * vaddr);
void get_arg(struct intr_frame *f,int *arg, int n);


void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

}

static void
syscall_handler (struct intr_frame *f)
{

  check_valid_ptr((const void *)f->esp);
  int arg[3];
  int syscall_num = *(int*)f->esp;
  switch (syscall_num) {
    case SYS_HALT:{
	    halt();
	    break;
    }
    case SYS_EXIT:{
	    get_arg(f, &arg[0], 1);
      // f->eax = arg[0];
	    exit(arg[0]);
	    break;
    }
    case SYS_EXEC:{
      get_arg(f, &arg[0], 1);
	    arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	    f->eax = exec((const char *) arg[0]);
	    break;
    }
    case SYS_WAIT:{
      get_arg(f, &arg[0], 1);
	    // f->eax = wait(arg[0]);
	    break;
    }
    case SYS_CREATE:{
      get_arg(f, &arg[0], 2);
      arg[0] = user_to_kernel_ptr((const void *) arg[0]);
      f->eax = create((const char *)arg[0], (unsigned) arg[1]);
	    break;
    }
    case SYS_REMOVE:{
	    get_arg(f, &arg[0], 1);
	    arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	    f->eax = remove((const char *) arg[0]);
	    break;
    }
    case SYS_OPEN:{
	    get_arg(f, &arg[0], 1);
	    arg[0] = user_to_kernel_ptr((const void *) arg[0]);
	    f->eax = open((const char *) arg[0]);
	    break;
    }
    case SYS_FILESIZE:{
	    get_arg(f, &arg[0], 1);
	    f->eax = filesize(arg[0]);
	    break;
    }
    case SYS_READ:{
	    get_arg(f, &arg[0], 3);
	    check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
	    arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	    f->eax = read(arg[0], (void *) arg[1], (unsigned) arg[2]);
	    break;
    }
    case SYS_WRITE:{
	    get_arg(f, &arg[0], 3);
	    check_valid_buffer((void *) arg[1], (unsigned) arg[2]);
      // arg[1] = user_to_kernel_ptr((const void *) arg[1]);
	    f->eax = write(arg[0], (const void *) arg[1],(unsigned) arg[2]);
	    break;
    }
    case SYS_SEEK:{
	    get_arg(f, &arg[0], 2);
	    seek(arg[0], (unsigned) arg[1]);
	    break;
    }
    case SYS_TELL:{
	    get_arg(f, &arg[0], 1);
      f->eax = tell(arg[0]);
      break;
    }
    case SYS_CLOSE:{
	    get_arg(f, &arg[0], 1);
	    close(arg[0]);
	    break;
    }
  }

}

static int read(int fd,void *buffer,unsigned size){
  // exit(0);
  return 5;
}

static int
write(int fd,const void* buffer, unsigned size){
  if(fd == STDOUT_FILENO){
    putbuf(buffer,size);
    return size;
  }
  return 5;
}


static pid_t exec(const char* cmd_line){
  // lock_acquire(&filesys_lock);
  tid_t tid = process_execute(cmd_line);

  struct thread* child_process = get_thread_by_id(tid);

  // if(!child_process)
    return -1;

  return tid;
}

static int wait(pid_t pid){
  return process_wait(pid);
}

static bool create(const char * file, unsigned initial_size){
  if(file==NULL)
    thread_exit();
  // lock_acquire(&filesys_lock);
  bool ret = filesys_create(file,initial_size);
  // lock_release(&filesys_lock);
  return ret;
}

static bool remove(const char* file){
  if(file==NULL)
    thread_exit();
  bool ret = filesys_remove(file);
  return ret;
}
static int filesize(int fd){
  // exit(0);
  return 5;
}
static void seek(int fd, unsigned position){
  // exit(0);
  return 5;
}
static unsigned tell(int fd){
  // exit(0);
  return 5;
}
static int open(const char* file){
  // exit(0);
  return 5;
}
static void close(int fd){
  // exit(0);
  return 5;
}

static void halt(void){
	power_off();
}

static void
exit(int status)
{
  printf("%s: exit(%d)\n",thread_current()->name,status);
  printf ("Execution of '%s' complete.\n", thread_current()->name);
  thread_exit();
}

void
get_arg(struct intr_frame *f,int *arg, int n){
  int i,*ptr;
  for(i=0;i<n;i++){
    ptr = (int*)f->esp + i + 1;
    check_valid_ptr((const void *) ptr);
    arg[i] = *ptr;
  }
}

int
user_to_kernel_ptr(const void * vaddr){
  check_valid_ptr(vaddr);
  void * ptr = pagedir_get_page(thread_current()->pagedir,vaddr);
  if(!ptr){
    exit(-1);
  }
  return (int) ptr;
}

void
check_valid_ptr(const void* vaddr){
if(!is_user_vaddr(vaddr) || vaddr <USER_VADDR_BOTTOM){
  // printf("Not valid user address ERROR\n");
    exit(-1);
  }
}

void check_valid_buffer(void * buffer, unsigned size){
  unsigned i;
  char* local_buffer = (char *)buffer;
  for(i=0;i<size;i++){
    check_valid_ptr((const void*)local_buffer);
    local_buffer++;
  }
}
