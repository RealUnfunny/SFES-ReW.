#include "comm.h"

void Addresser(box_t *box)
{
  box->address = 0xF0F0F0F000LL;
  box->address |= (uint64_t)(box->index)[0];
  box->address |= (uint64_t)(box->index)[1] << 8;
}
