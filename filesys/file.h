#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H

#include "filesys/off_t.h"
#include "lib/kernel/list.h"
#include "threads/synch.h"
// typedef int bool;
struct inode;
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    int deny_write;            /* Has file_deny_write() been called? */
    // 取消这样的修改，可能导致访问内核内存
    // 邱维东的修改
    // int fd;// 添加一个文件描述符的属性
    // struct list_elem elem;
  };
/*
 * 定义一个文件描述符的结构
*/
struct file_descriptor
{
   int fd;
   struct file * f;
   // 加两个锁
   struct lock in;
   struct lock out;
   struct list_elem elem;
};
/* Opening and closing files. */
struct file *file_open (struct inode *);
struct file *file_reopen (struct file *);
void file_close (struct file *);
struct inode *file_get_inode (struct file *);

/* Reading and writing. */
off_t file_read (struct file *, void *, off_t);
off_t file_read_at (struct file *, void *, off_t size, off_t start);
off_t file_write (struct file *, const void *, off_t);
off_t file_write_at (struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write (struct file *);
void file_allow_write (struct file *);

/* File position. */
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);
off_t file_length (struct file *);

#endif /* filesys/file.h */
