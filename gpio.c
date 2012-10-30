/* L-CardA gpio driver */
#define MODULE
#define __KERNEL__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/vr41xx/toadkk-tcs8000.h>
#include <asm/io.h>

#ifndef GPIO_MAJOR
#define GPIO_MAJOR	17
#endif
#define GPMODE0	VR4181A_GPMODE0 /*0x00 b300*/
#define GPDATA0 	INTCS(0x00b310) /*0x00 b310*/

static char gpio_pin_usable[64]=
{ /* 0=not usable 1=output 2=input 3=both */
  3,3,3,3, 3,3,3,3, 3,0,0,0, 0,3,3,0,   	/* 0 - 15 */
  0,0,0,0, 3,3,3,3, 0,0,0,0, 0,0,0,0,	/* 16 - 31 */
  0,0,0,0, 0,0,0,0, 3,3,3,3, 3,3,3,3,   	/* 32 - 47 */
  3,3,0,0, 3,0,0,0, 0,0,0,0, 0,0,0,0,	/* 48 - 63 */
};

/* Typedef */
typedef struct _gpio_cont {
  int bit;
} GpioCont;

/* Global */
int g_gpio_major = GPIO_MAJOR;

/* Functions */
/* -------------------------------------------------------------------
// gpio_open
// デバイスを開く
// Minorナンバーが操作するGPIOの番号を表す
-------------------------------------------------------------------- */
int gpio_open(struct inode *inode, struct file *filp)
{

  int minor = MINOR(inode->i_rdev);
  int bit = minor & 0x3f; /*下６ビットをbit選択に使用*/

  GpioCont *cont;

  if((gpio_pin_usable[bit] & 0x03) == 0x00)
    return -NODEV;
  cont = (GpioCont *)kmalloc(sizeof(GpioCont), GFP_KERNEL);
  cont->bit = bit;
  filp->private_data = cont;
  MOD_INC_USE_COUNT;
  return 0;
}
/* -------------------------------------------------------------------
// gpio_release
// デバイスを閉じる
-------------------------------------------------------------------- */
int gpio_release(struct inode *inode, struct file *filp)
{
  GpioCont *cont = (GpioCont *)filp->private_data;
  kfree(cont);
  MOD_DEC_USE_COUNT;
  return 0;
}
/* -------------------------------------------------------------------
// gpio_read
// 読み込み - ポートの値を読み出す
-------------------------------------------------------------------- */

ssize_t gpio_read(struct file *filp, const char *buf,size_t count,
	loff_t *f_pos)
	{
	
	return 0;
}


/* -------------------------------------------------------------------
// gpio_write
// 書き込み - ポートに出力する
-------------------------------------------------------------------- */
ssize_t gpio_write(struct file *filp, const char *buf,size_t count,
	loff_t *f_pos)
{
  GpioCont *cont = (GpioCont *)filp->private_data;
  int bit     = cont->bit;
  volatile unsigned short *mode_port;
  volatile unsigned short *data_port;
  unsigned short temp,data,mask,prev;
  int i;
  /*使用できるかチェック*/
  if(!(gpio_pin_usable[bit] & 0x01))
    return -EFAULT;
  /*使用するレジスタのアドレスを計算*/
  mode_port = (unsigned short *)(GPMODE0 + (bit / 8)*2);
  data_port = (unsigned short *)(GPDATA0 + (bit / 16));
  /*モードレジスタの設定すべき位置を計算*/
  mask = 3 << ((bit % 8) * 2);
  data = 3 << ((bit % 8) * 2);
  /*モードレジスタの該当ビットを設定して出力モードにする*/
  prev = readw(mode_port);
  temp = (prev & ~mask) | (data & mask);
  writew(temp,mode_port);
  /*データレジスタの設定すべき位置を計算*/
  mask = 1 << (bit % 16);
  /*データを出力*/
  prev= readw(data_port);
  temp = (prev & ~mask) | (data & mask);
  for(i=0;i<count;i++){
    int c;
    if(get_user(c, buf+i)) return -EFAULT;
    if(c==' ' || c=='\t' || c=='\n' || c=='\r') continue;
    if(c=='\0') break;
    if(c=='0'){
      temp = (prev & ~mask);
    }else{
      temp = (prev & ~mask) | mask;
    }
    writew(temp,data_port);
  }
  *f_pos += count;
  return count;
}
/* ファイルオペレーション定義 */
struct file_operations gpio_fops = {
  read:    gpio_read,
  write:   gpio_write,
  open:    gpio_open,
  release: gpio_release,
};
/* -------------------------------------------------------------------
// init_module
// モジュールの初期化
-------------------------------------------------------------------- */
int init_module(void)
{
  int result;
  result = register_chrdev((g_gpio_major), "gpio", &gpio_fops);
  if(result < 0){
    printk(KERN_WARNING "gpio: error register_chrdev(%d)\n", (g_gpio_major));
    return result;
  }
  if(g_gpio_major == 0) g_gpio_major = result;
  printk(KERN_INFO "gpio installed\n");
  return 0;
}
/* -------------------------------------------------------------------
// cleanup_module
// モジュールの解放
-------------------------------------------------------------------- */
void cleanup_module(void)
{
  int result;
  result = unregister_chrdev(g_gpio_major, "gpio");
  if(result < 0){
    printk(KERN_ALERT "gpio: unregister major %d failed\n", g_gpio_major);
    return;
  }
  printk(KERN_INFO "gpio uninstalled\n");
}











