#include "log.h"

using namespace std;

CLog::CLog() {
   string name = getDate() + string(".log");

   file = fopen(name.c_str(), "w");
   if(!file)
      fprintf(stderr, "error creating log file");

   _logLevel = ERROR;
   displayMessages = false;
}


CLog::CLog(const char * filename) {
   string name = getDate() + string("_") + string(filename) + string(".log");

   file = fopen(name.c_str(), "w");
   if(!file)
      fprintf(stderr, "error creating log file");

   _logLevel = ERROR;
   displayMessages = false;
}

void CLog::log(LOGLEVEL logLevel, const char *msg, ...)
{
   va_list ap;
   va_start(ap, msg);

   char buf[64];

   vsprintf(buf, msg, ap);
   va_end(ap);

   if(logLevel <= _logLevel) {
      fprintf(file, buf);
      fprintf(file, "\n");

      if(displayMessages) {
         fprintf(stdout, buf);
         fprintf(stdout, "\n");
      }

   }
}

CLog::~CLog() {
   if(file)
      fclose(file);
}

const string CLog::getDate() {
   time_t t = time(0);
   struct tm now = *localtime(&t);
   char buf[80];
   strftime(buf, sizeof(buf),"%y%m%d%H%M%S", &now);
   return buf;
}

const string CLog::getTime() {
   time_t t = time(0);
   struct tm now = *localtime(&t);
   char buf[20];
   strftime(buf, sizeof(buf),"%H%M%S", &now);
   return buf;
}
