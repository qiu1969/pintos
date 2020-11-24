#include "userprog/syscall.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "lib/stdio.h"
#include "lib/kernel/stdio.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "process.h"
#include "lib/string.h"
#include "devices/input.h"
#include "userprog/pagedir.h"
#include "threads/malloc.h"

static void syscall_handler(struct intr_frame *);

// 查找file的辅助函数,返回一个文件描述符结构体的指针
struct file_descriptor *
getFile(int fd)
{
  struct list_elem *l;
  for (l = list_begin(&thread_current()->files); l != list_end(&thread_current()->files); l = list_next(l))
  {
    struct file_descriptor *f = list_entry(l, struct file_descriptor, elem);
    if (f->fd == fd)
    {
      return f;
    }
  }
  return NULL;
}

bool address_valid(void * vaddr)
{
  return is_user_vaddr(vaddr) && !is_kernel_vaddr(vaddr)
    && pagedir_get_page(thread_current()->pagedir,vaddr)!=NULL;
}

bool string_valid(char * vaddr)
{
  while(address_valid(vaddr)&&(*vaddr)!='\0')
    vaddr++;
  return address_valid(vaddr);
}

void syscall_init(void)
{
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler(struct intr_frame *f UNUSED)
{
  // printf("...syscall\n");
  if(f == NULL || !address_valid(f->esp) || !address_valid(f->esp+3)) exit(-1);
  switch (*(int *)f->esp)
  {
  case SYS_HALT:
  {
    // 实现系统调用halt,已经通过
    // printf("...SYS_CODE : SYS_HALT\n");
    halt();
    break;
  }
  case SYS_EXIT:
  {
    // printf("...SYS_CODE : SYS_EXIT\n");
    // 实现系统调用void exit(int status); 貌似可以了
    // 要检查参数是否在有效地址
    if(!address_valid((int *)f->esp+1)) exit(-1);
    int status = *((int *)f->esp + 1);
    exit(status);
    break;
  }
  case SYS_EXEC:
  {
    // printf("...SYS_CODE : SYS_EXEC\n");
    if(!address_valid(f->esp+4) || !address_valid(f->esp+5)||!address_valid(f->esp+6)||!address_valid(f->esp+7)) exit(-1);
    char *file_name = (char *)(*((int *)f->esp + 1));
    if(!string_valid(file_name)) exit(-1);
    f->eax = exec(file_name);
    break;
  }
  case SYS_WAIT:
  {
    // printf("...SYS_CODE : SYS_WAIT\n");
    // pid_t就是int
    if(!address_valid((int *)f->esp+1)) exit(-1);
    pid_t pid = *((int *)f->esp + 1);
    f->eax = wait(pid);
    break;
  }

  case SYS_CREATE:
  {
    // printf("...SYS_CODE : SYS_CREATE\n");
    if(!address_valid((int *)f->esp+1) || !address_valid((int *)f->esp+2)) exit(-1);
    char *file = (char *)(*((int *)f->esp + 1));
    if(!address_valid(file)) exit(-1);
    unsigned int initial_size = *((int *)f->esp + 2);
    f->eax = create(file, initial_size);
    break;
  }
  case SYS_REMOVE:
  {
    // printf("...SYS_CODE : SYS_REMOVE\n");
    if(!address_valid((int *)f->esp+1)) exit(-1);
    char *file = (char *)(*((int *)f->esp + 1));
    if(!address_valid(file)) exit(-1);
    f->eax = remove(file);
    break;
  }
  case SYS_OPEN:
  {
    // printf("...SYS_CODE : SYS_OPEN\n");
    if(!address_valid((int *)f->esp+1)) exit(-1);
    char *file = (char *)(*((int *)f->esp + 1));
    if(!address_valid(file)) exit(-1);
    f->eax = open(file);
    break;
  }
  case SYS_FILESIZE:
  {
    // printf("...SYS_CODE : SYS_FILESIZE\n");
    if(!address_valid((int *)f->esp+1)) exit(-1);
    int fd = *((int *)f->esp + 1);
    f->eax = filesize(fd);
    break;
  }
  case SYS_READ:
  {
    // printf("...SYS_CODE : SYS_READ\n");
    if(!address_valid((int *)f->esp+1)||!address_valid((int *)f->esp+2)||!address_valid((int *)f->esp+3)) exit(-1);
    int fd = *((int *)f->esp + 1);
    void *buffer = (void *)(*((int *)f->esp + 2));
    if(!address_valid(buffer)) exit(-1);
    unsigned int length = *((unsigned *)f->esp + 3);
    f->eax = read(fd, buffer, length);
    break;
  }
  case SYS_WRITE:
  {
    // printf("...SYS_CODE : SYS_WRITE\n");
    if(!address_valid((int *)f->esp+1)||!address_valid((int *)f->esp+2)||!address_valid((int *)f->esp+3)) exit(-1);
    int fd = *((int *)f->esp + 1);
    void *buffer = (void *)(*((int *)f->esp + 2));
    if(!address_valid(buffer)) exit(-1);
    unsigned size = *((unsigned *)f->esp + 3);
    // 运行你编写的系统调用函数。
    // 一旦系统调用要返回一个值，将它保存在eax中
    f->eax = write(fd, buffer, size);
    break;
  }
  case SYS_SEEK:
  {
    // printf("...SYS_CODE : SYS_SEEK\n");
    if(!address_valid((int *)f->esp+1)||!address_valid((int *)f->esp+2)) exit(-1);
    int fd = *((int *)f->esp + 1);
    unsigned int position = *((int *)f->esp + 2);
    seek(fd, position);
    break;
  }
  case SYS_TELL:
  {
    // printf("...SYS_CODE : SYS_TELL\n");
    if(!address_valid((int *)f->esp+1)) exit(-1);
    int fd = *((int *)f->esp + 1);
    f->eax = tell(fd);
    break;
  }
  case SYS_CLOSE:
  {
    // printf("...SYS_CODE : SYS_CLOSE\n");
    if(!address_valid((int *)f->esp+1)) exit(-1);
    int fd = *((int *)f->esp + 1);
    close(fd);
    break;
  }
  default:
    // printf("other!\n");
    break;
  }
}

void halt(void)
{
  // 直接断电就好
  shutdown_power_off();
}
void exit(int status)
{
  // 设置退出状态，然后退出
  // printf("...exit is called!\n");
  struct thread *cur = thread_current();
  cur->ret = status;
  thread_exit();
  // printf("...leave exit!\n");
  // process_exit();
}
/*
 * 运行名称为cmd_line的可执行文件，并传递所有给定的参数，并返回新进程的程序ID（pid）。 
 * 如果程序由于任何原因无法加载或运行，则必须返回pid -1，否则不应为有效pid。 
 * 因此，父进程无法从exec返回，直到它知道子进程是否成功加载了其可执行文件。 
 * 您必须使用适当的同步来确保这一点
*/
pid_t exec(const char *file)
{
  // printf("...exec\n");
  if(file == NULL || !is_user_vaddr(file)) return TID_ERROR;
  tid_t tid = process_execute(file);
  if(tid == TID_ERROR) return TID_ERROR;
  // 不能返回，要等待子线程加载完成
  // 等待
  // sema_down(&thread_current()->wait_for_child);
  return tid;
}
int wait(pid_t pid)
{
  // printf("wait:%d\n", pid);
  return process_wait(pid);
}
bool create(const char *file, unsigned initial_size)
{
  // 直接使用filesys_create函数创建文件
  if(file == NULL||!is_user_vaddr(file)) exit(-1);
  return filesys_create(file, initial_size);
}
bool remove(const char *file)
{
  // 直接使用filesys_remove删除文件，删除文件后，打开的文件不会被关闭
  return filesys_remove(file);
}
int open(const char *file)
{
  if(file == NULL || !is_user_vaddr(file)) exit(-1);
  static int next_fd = 2;
  struct file *f = filesys_open(file);
  if (f == NULL)
    return  -1;
  struct file_descriptor * descriptor = calloc(1,sizeof(struct file_descriptor));
  descriptor->fd = next_fd++; // 为打开的文件分配文件描述符
  descriptor->f = f;
  list_push_back(&thread_current()->files, &descriptor->elem);// 将打开的文件添加到当前线程的文件列表当中
  return descriptor->fd;
}
int filesize(int fd)
{
  struct file_descriptor * descriptor = getFile(fd);
  if(descriptor == NULL) exit(-1);
  return file_length(descriptor->f);
}
int read(int fd, void *buffer, unsigned length)
{
  if(buffer == NULL || !is_user_vaddr(buffer)) return -1;
  if (fd == STDIN_FILENO)
  {
    for(int i=0;i<length;i++)
    {
      char c = input_getc();
      memcpy(buffer+i,&c,sizeof(char));
      return length;
    }
  }
  else
  {
    struct file_descriptor *descriptor = getFile(fd);
    if(descriptor == NULL) return  -1;
    return (int)file_read(descriptor->f, buffer, length);
  }
}
int write(int fd, const void *buffer, unsigned length)
{
  if(buffer == NULL || !is_user_vaddr(buffer)) return 0;
  if (fd == STDOUT_FILENO) // 如果输出到终端
  {
    // 直接使用putbuf即可输出
    putbuf(buffer, length);
    return length;
  }
  else
  {
    // printf("write:%d %d\n", fd, length);
    struct file_descriptor *descriptor = getFile(fd);
    if(descriptor == NULL) return 0;
    return (int)file_write(descriptor->f, buffer, length);
  }
}
void seek(int fd, unsigned position)
{
  // printf("seek:%d %d\n", fd, position);
  struct file_descriptor *descriptor = getFile(fd);
  if(descriptor == NULL) return ;
  file_seek(descriptor->f, position);
}
unsigned tell(int fd)
{
  struct file_descriptor *descriptor = getFile(fd);
  if(descriptor == NULL) return 0;
  return file_tell(descriptor->f);
}

void close(int fd)
{
  struct file_descriptor * descriptor = getFile(fd);
  if(descriptor == NULL) return;
  file_close(descriptor->f);
  // 使用list_remove崩溃的原因，elem->prev是一个kernel地址，
  // 对它解引用elem->prev->next会导致崩溃
  if(descriptor != NULL)
    list_remove(&descriptor->elem);
  free(descriptor);
}
