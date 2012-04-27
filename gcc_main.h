
#include "misc.h"
#include "compiler_specific_func.h"
#include "engine.h"


///
#define ID_MENUITEM_reloadBegin _APS_NEXT_COMMAND_VALUE


// global variable
extern int __argc;
extern _TCHAR** __targv;

class Mayu
{
  Engine m_engine;				/// engine

  tomsgstream m_log;				/** log stream (output to log
                                                    dialog's edit) */
  
  Setting *m_setting;				/// current setting
  time_t m_startTime;				/// mayu started at ...
public:

  /// load setting
  bool load()
  {
    Setting *newSetting = new Setting;

    // check args
    for (int i = 1; i < __argc; ++ i)
    {
      // set symbol
      if (__targv[i][0] == _T('-') && __targv[i][1] == _T('D'))
        newSetting->m_symbols.insert(__targv[i] + 2);
      else if (_tcsicmp(__targv[i], "-v") == 0 || _tcsicmp(__targv[i], "--verbose") == 0)
        m_log.setDebugLevel(1);
    }

#if defined(WIN32)
    newSetting->m_symbols.insert("WINDOWS");
#elif defined(__linux__)
    newSetting->m_symbols.insert("LINUX");
#elif defined(__APPLE__)
    newSetting->m_symbols.insert("MAC");
#endif

    // load setting
    if (!SettingLoader(&m_log, &m_log).load(newSetting))
    {
      delete newSetting;
      Acquire a(&m_log, 0);
      m_log << _T("error: failed to load.") << std::endl;
      return false;
    }
    m_log << _T("successfully loaded.") << std::endl;
    while (!m_engine.setSetting(newSetting))
#if defined(__linux__)
      sleep(1);
#  elif defined(__APPLE__)
    sleep(1);
#endif
    delete m_setting;
    m_setting = newSetting;
    return true;
  }

  Mayu()
    :
#if defined(__linux__) || defined(__APPLE__)
    m_log(),
#endif
    m_setting(NULL),
    m_engine(m_log)
  {
#ifdef _DEBUG
    m_log.setDebugLevel(1);
#else

#endif
    time(&m_startTime);
#if 0
    HomeDirectories pathes;
    ggetHomeDirectories(&pathes);
    for (HomeDirectories::iterator i = pathes.begin(); i != pathes.end(); ++ i)
      if (SetCurrentDirectory(i->c_str()))
        break;
#endif  
  }

  ///
  ~Mayu()
  {
    // stop keyboard handler thread
    m_engine.stop();
    
    // remove setting;
    delete m_setting;
  }
	
  void taskLoop()
  {
    m_engine.start();
  }
	
};
