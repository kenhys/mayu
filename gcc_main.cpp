// Linux,Darwin用のmaine

#define APSTUDIO_INVOKED

#include "misc.h"
#include "compiler_specific_func.h"
#include "engine.h"
#include "errormessage.h"
#include "function.h"
#include "mayu.h"
#if defined(WIN32)
#include "mayurc.h"
#endif
#include "msgstream.h"
#include "multithread.h"
#include "setting.h"
#include <time.h>
#include "gcc_main.h"


#if defined(__linux__) || defined(__APPLE__)
// TODO:

int main(int argc, char* argv[])
{
  __argc = argc;
  __targv = argv;
	
  //TODO: 多重起動 check
	
  try
  {
    Mayu mayu;

    // 設定ファイルのロード
    if (mayu.load())
    {
      //コマンド実行時のEnterが残る可能性があるため、ちょっとだけ待つ
      sleep(2);
		
      // キー置き換えの実行
      mayu.taskLoop();			
    }
  }
  catch (ErrorMessage e)
  {
    fprintf(stderr, "%s\n", e.getMessage().c_str());
  }
	
  return 0;
}

#endif
