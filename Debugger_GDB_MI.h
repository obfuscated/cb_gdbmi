/***************************************************************
 * Name:      debbugger_gdbmi
 * Purpose:   Code::Blocks plugin
 * Author:    Teodor Petrov a.k.a obfuscated (fuscated@gmail.com)
 * Created:   2009-06-20
 * Copyright: Teodor Petrov a.k.a obfuscated
 * License:   GPL
 **************************************************************/

#ifndef DEBUGGER_GDB_MI_H_INCLUDED
#define DEBUGGER_GDB_MI_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <tr1/memory>
#include <cbplugin.h> // for "class cbPlugin"

#include "cmd_queue.h"
#include "definitions.h"
#include "events.h"
#include "gdb_executor.h"

//class PipedProcess;
class TextCtrlLogger;
class Compiler;


class Debugger_GDB_MI : public cbDebuggerPlugin
{
    public:
        /** Constructor. */
        Debugger_GDB_MI();
        /** Destructor. */
        virtual ~Debugger_GDB_MI();

    public:
        virtual void ShowToolMenu();
        virtual bool ToolMenuEnabled() const { return true; }

        /** Invoke configuration dialog. */
        virtual int Configure();

        /** Return the plugin's configuration priority.
          * This is a number (default is 50) that is used to sort plugins
          * in configuration dialogs. Lower numbers mean the plugin's
          * configuration is put higher in the list.
          */
        virtual int GetConfigurationPriority() const { return 50; }

        /** Return the configuration group for this plugin. Default is cgUnknown.
          * Notice that you can logically OR more than one configuration groups,
          * so you could set it, for example, as "cgCompiler | cgContribPlugin".
          */
        virtual int GetConfigurationGroup() const { return cgUnknown; }

        /** Return plugin's configuration panel.
          * @param parent The parent window.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetConfigurationPanel(wxWindow* parent){ return 0; }

        /** Return plugin's configuration panel for projects.
          * The panel returned from this function will be added in the project's
          * configuration dialog.
          * @param parent The parent window.
          * @param project The project that is being edited.
          * @return A pointer to the plugin's cbConfigurationPanel. It is deleted by the caller.
          */
        virtual cbConfigurationPanel* GetProjectConfigurationPanel(wxWindow* /*parent*/, cbProject* /*project*/){ return 0; }

        virtual bool Debug(bool breakOnEntry);
        virtual void Continue();
        virtual bool RunToCursor(const wxString& filename, int line, const wxString& line_text);
        virtual void SetNextStatement(const wxString& filename, int line);
        virtual void Next();
        virtual void NextInstruction();
        virtual void StepIntoInstruction();
        virtual void Step();
        virtual void StepOut();
        virtual void Break();
        virtual void Stop();
        virtual bool IsRunning() const;
        virtual bool IsStopped() const;
        virtual int GetExitCode() const;
        void SetExitCode(int code) { m_exit_code = code; }

		// stack frame calls;
		virtual int GetStackFrameCount() const;
		virtual const cbStackFrame& GetStackFrame(int index) const;
		virtual void SwitchToFrame(int number);
		virtual int GetActiveStackFrame() const;

        // breakpoints calls
        virtual cbBreakpoint* AddBreakpoint(const wxString& filename, int line);
        virtual cbBreakpoint* AddDataBreakpoint(const wxString& dataExpression);
        virtual int GetBreakpointsCount() const;
        virtual cbBreakpoint* GetBreakpoint(int index);
        virtual const cbBreakpoint* GetBreakpoint(int index) const;
        virtual void UpdateBreakpoint(cbBreakpoint *breakpoint);
        virtual void DeleteBreakpoint(cbBreakpoint* breakpoint);
        virtual void DeleteAllBreakpoints();
        virtual void ShiftBreakpoint(int index, int lines_to_shift);

        // threads
        virtual int GetThreadsCount() const;
        virtual const cbThread& GetThread(int index) const;
        virtual bool SwitchToThread(int thread_number);

        // watches
        virtual cbWatch* AddWatch(const wxString& symbol);
        virtual void DeleteWatch(cbWatch *watch);
        virtual bool HasWatch(cbWatch *watch);
        virtual void ShowWatchProperties(cbWatch *watch);
        virtual bool SetWatchValue(cbWatch *watch, const wxString &value);
        virtual void ExpandWatch(cbWatch *watch);
        virtual void CollapseWatch(cbWatch *watch);

        virtual void SendCommand(const wxString& cmd, bool debugLog);

        virtual void AttachToProcess(const wxString& pid);
        virtual void DetachFromProcess();
        virtual bool IsAttachedToProcess() const;

        virtual void GetCurrentPosition(wxString &filename, int &line);
        virtual void RequestUpdate(DebugWindows window);
    protected:
        /** Any descendent plugin should override this virtual method and
          * perform any necessary initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded and should attach in Code::Blocks. When Code::Blocks
          * starts up, it finds and <em>loads</em> all plugins but <em>does
          * not</em> activate (attaches) them. It then activates all plugins
          * that the user has selected to be activated on start-up.\n
          * This means that a plugin might be loaded but <b>not</b> activated...\n
          * Think of this method as the actual constructor...
          */
        virtual void OnAttachReal();

        /** Any descendent plugin should override this virtual method and
          * perform any necessary de-initialization. This method is called by
          * Code::Blocks (PluginManager actually) when the plugin has been
          * loaded, attached and should de-attach from Code::Blocks.\n
          * Think of this method as the actual destructor...
          * @param appShutDown If true, the application is shutting down. In this
          *         case *don't* use Manager::Get()->Get...() functions or the
          *         behaviour is undefined...
          */
        virtual void OnReleaseReal(bool appShutDown);

    protected:
        virtual void ConvertDirectory(wxString& /*str*/, wxString /*base*/, bool /*relative*/) {}
        virtual cbProject* GetProject() { return m_project; }
        virtual void ResetProject() { m_project = NULL; }
        virtual void CleanupWhenProjectClosed(cbProject *project);
        virtual void CompilerFinished(bool compilerFailed);

    public:
        void UpdateWhenStopped();
        void UpdateOnFrameChanged(bool wait);
        dbg_mi::CurrentFrame& GetCurrentFrame() { return m_current_frame; }
    private:
        DECLARE_EVENT_TABLE();

        void OnGDBOutput(wxCommandEvent& event);
        void OnGDBTerminated(wxCommandEvent& event);

        void OnTimer(wxTimerEvent& event);
        void OnIdle(wxIdleEvent& event);

        void OnMenuInfoCommandStream(wxCommandEvent& event);

        int LaunchDebugger(wxString const &debugger, wxString const &debuggee, wxString const &working_dir,
                           int pid, bool console);

    private:
        void AddStringCommand(wxString const &command);
        void DoSendCommand(const wxString& cmd);
        void RunQueue();
        void ParseOutput(wxString const &str);

        bool SelectCompiler(cbProject &project, Compiler *&compiler,
                            ProjectBuildTarget *&target, long pid_to_attach);
        int StartDebugger(cbProject *project);
        void CommitBreakpoints(bool force);
        void CommitRunCommand(wxString const &command);
        void CommitWatches();

        void KillConsole();

    private:
        int m_page_index, m_dbg_page_index;
        TextCtrlLogger  *m_log, *m_debug_log;
        wxTimer m_timer_poll_debugger;
        cbProject *m_project;

        dbg_mi::GDBExecutor m_executor;
        dbg_mi::ActionsMap  m_actions;
        dbg_mi::LogPaneLogger m_execution_logger;

        typedef std::vector<dbg_mi::Breakpoint::Pointer> Breakpoints;

        Breakpoints m_breakpoints;
        Breakpoints m_temporary_breakpoints;
        dbg_mi::BacktraceContainer m_backtrace;
        dbg_mi::ThreadsContainer m_threads;
        dbg_mi::WatchesContainer m_watches;

        dbg_mi::CurrentFrame m_current_frame;
        int m_exit_code;
        int m_console_pid;
        int m_pid_attached;
};
#endif // DEBUGGER_GDB_MI_H_INCLUDED
