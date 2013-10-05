#include <fstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <stdarg.h>

/*Log Levels are
  ERROR, SERVER, POT, HAND, VERBOSE
*/

enum LOGLEVEL {
      ERROR, SERVER, POT, HAND, VERBOSE
};

class CLog {
public:
   CLog();
   
   CLog(const char * filename);

   void setLogLevel(LOGLEVEL logLevel) { _logLevel = logLevel; };

   void log(LOGLEVEL logLevel, const char *msg, ...);

   void setDisplayMessages(bool b) { displayMessages = b; };
   ~CLog();
private:
   FILE * file;

   LOGLEVEL _logLevel;

   bool displayMessages;

   const std::string getDate();

   const std::string getTime();
};
