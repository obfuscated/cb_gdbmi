#ifndef _DEBUGGER_GDB_MI_GDB_CONFIG_H_
#define _DEBUGGER_GDB_MI_GDB_CONFIG_H_

#include <debuggermanager.h>

//(*Headers(ConfigurationPanel)
#include <wx/panel.h>
class wxTextCtrl;
class wxStaticBoxSizer;
class wxButton;
class wxBoxSizer;
class wxStaticText;
class wxCheckBox;
//*)

namespace dbg_mi
{
class Configuration;

class ConfigurationPanel: public wxPanel
{
    friend class Configuration;
public:

    ConfigurationPanel(wxWindow* parent);
    virtual ~ConfigurationPanel();

private:

    //(*Declarations(ConfigurationPanel)
    wxTextCtrl* m_initial_commands;
    wxTextCtrl* m_exec_path;
    wxCheckBox* m_check_cpp_excepetions;
    wxCheckBox* m_check_pretty_printers;
    //*)

    //(*Identifiers(ConfigurationPanel)
    static const long ID_TEXTCTRL_EXEC_PATH;
    static const long ID_BUTTON_BROWSE;
    static const long ID_TEXTCTRL_INIT_COMMANDS;
    static const long ID_CHECKBOX_PRETTY_PRINTERS;
    static const long ID_CHECKBOX_CPP_EXCEPTIONS;
    //*)

    //(*Handlers(ConfigurationPanel)
    //*)

    DECLARE_EVENT_TABLE()
};

class Configuration : public cbDebuggerConfiguration
{
public:
    Configuration(const ConfigManagerWrapper &config);

    virtual cbDebuggerConfiguration* Clone() const;

    virtual wxPanel* MakePanel(wxWindow *parent);
    virtual bool SaveChanges(wxPanel *panel);
public:
    wxString GetDebuggerExecutable();
    wxArrayString GetInitialCommands();
    wxString GetInitialCommandsString();

    enum Flags
    {
        PrettyPrinters = 0,
        CatchCppExceptions
    };

    bool GetFlag(Flags flag);


};

} // namespace dbg_mi

#endif // _DEBUGGER_GDB_MI_GDB_CONFIG_H_

