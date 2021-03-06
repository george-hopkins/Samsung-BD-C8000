/* Test driver for thwchar
 *
 * $Id: test_thwchar.c,v 1.5 2004/10/12 09:27:19 thep Exp $
 */

#define MAXLINELENGTH 1000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thai/thwchar.h>

static const thchar_t win_sample[] = "�\x8bͻ�\x86���\x82\x9b�\xfc\x80�\x90ء�һ\x99ҡ��һ\x99\x9cһ\x99� \x8c�\x8b ա�\x9a";
static const thchar_t mac_sample[] = "�\x88ͻ�\x83���\x95\x98�\xd8\xb0�\xadء�һ\x8fҡ��һ\x8f\x99һ\x8f� \x89�\x88 ա�\x93";

int main (int argc, char* argv[])
{
  thchar_t  tis_620_0[MAXLINELENGTH];
  thchar_t  newtis_620_0[MAXLINELENGTH];
  thwchar_t uni[MAXLINELENGTH];
  int       outputLength;
  const thchar_t *pc;
  int       err = 0;

  strcpy(tis_620_0,"���ʴդ�Ѻ ����繡�÷��ͺ����ͧ");
  
  fprintf(stderr, "Testing thwchar...\n");
  fprintf(stderr, "Input:  tis-620-0 string of length %d: %s\n",
          strlen(tis_620_0), tis_620_0);

  /* tis-620-0 to Unicode conversion */
  outputLength = th_tis2uni_line(tis_620_0, uni, MAXLINELENGTH);
  fprintf(stderr, "Output: Unicode string of length %d:", wcslen(uni));

  /* Unicode to tis-620-0 conversion */
  fprintf(stderr, "\nConvert back to tis-620-0 string...\n");
  outputLength = th_uni2tis_line(uni, newtis_620_0, MAXLINELENGTH);
  fprintf(stderr, "Output: tis-620-0 string of length %d: %s\n",
          strlen(newtis_620_0), newtis_620_0);

  if (strcmp(newtis_620_0, tis_620_0) == 0) {
    fprintf(stderr, " Input = output, correct! Test thwchar OK.\n");
  } else {
    fprintf(stderr, " Input != output, incorrect!!\n");
    err = 1;
  }

  /* Test Unicode <-> Win/Mac extensions */
  for (pc = win_sample; *pc; ++pc) {
    if (th_uni2winthai(th_winthai2uni(*pc)) != *pc) {
      fprintf(stderr, "Inconsistent uni<->winthai conv: %02x -> %04lx, %02x\n",
              *pc, th_winthai2uni(*pc), th_uni2winthai(th_winthai2uni(*pc)));
      err = 1;
    }
  }
  for (pc = mac_sample; *pc; ++pc) {
    if (th_uni2macthai(th_macthai2uni(*pc)) != *pc) {
      fprintf(stderr, "Inconsistent uni<->macthai conv: %02x -> %04lx, %02x\n",
              *pc, th_macthai2uni(*pc), th_uni2macthai(th_macthai2uni(*pc)));
      err = 1;
    }
  }

  return err;
}
