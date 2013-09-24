#include "pack.h"


void packi16(unsigned char *buf, unsigned int i) {
   *buf++ = i>>8;  *buf++ = i;
}

unsigned int unpacki16(unsigned char *buf) {
   return (buf[0]<<8) | buf[1];
}

int8_t pack(unsigned char *buf, const char *format, ...) {
   va_list ap;
   int16_t h;
   int8_t b;
   char *s;
   int8_t size = 0, len;
   unsigned char *start;

   va_start(ap, format);

   // size header
   start = buf;
   *buf++ = size;

   for(; *format != '\0'; format++) {
      switch(*format) {
      case 'h':
         size += 2;
         h = (int16_t)va_arg(ap, int);
         packi16(buf, h);
         buf += 2;
         break;

      case 'b':
         size += 1;
         b = (int8_t)va_arg(ap, int);
         *buf++ = b;
         break;

      case 's':
         s = va_arg(ap, char*);
         len = strlen(s);
         size += len + 2;
         packi16(buf, len);
         buf += 2;
         memcpy(buf, s, len);
         buf += len;
         break;
      }
   }

   va_end(ap);

   *start = size;
   
   // return total packet size (incl. header);
   return size + 1;
}

void unpack(unsigned char *buf, const char *format, ...) {
   va_list ap;
   int16_t *h;
   int8_t *b;
   char *s;
   int32_t len, count, maxstrlen=0;

   va_start(ap, format);

   for(; *format != '\0'; format++) {
      switch(*format) {
      case 'h':
         h = va_arg(ap, int16_t*);
         *h = unpacki16(buf);
         buf += 2;
         break;

      case 'b':
         b = va_arg(ap, int8_t*);
         *b = *buf++;
         break;

      case 's':
         s = va_arg(ap, char*);
         len = unpacki16(buf);
         buf += 2;
         if (maxstrlen > 0 && len > maxstrlen) count = maxstrlen - 1;
         else count = len;
         memcpy(s, buf, count);
         s[count] = '\0';
         buf += len;
         break;

      default:
         if(isdigit(*format)) {
            maxstrlen = maxstrlen * 10 + (*format-'0');
         }
      }

      if (!isdigit(*format)) maxstrlen = 0;
   }

   va_end(ap);
}
