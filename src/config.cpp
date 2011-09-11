#include "config.h"

//(*InternalHeaders(ConfigurationPanel)
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
//*)

namespace dbg_mi
{

//(*IdInit(ConfigurationPanel)
const long ConfigurationPanel::ID_TEXTCTRL_EXEC_PATH = wxNewId();
const long ConfigurationPanel::ID_BUTTON_BROWSE = wxNewId();
const long ConfigurationPanel::ID_TEXTCTRL_INIT_COMMANDS = wxNewId();
const long ConfigurationPanel::ID_CHECKBOX_PRETTY_PRINTERS = wxNewId();
const long ConfigurationPanel::ID_CHECKBOX_CPP_EXCEPTIONS = wxNewId();
//*)

BEGIN_EVENT_TABLE(ConfigurationPanel,wxPanel)
	//(*EventTable(ConfigurationPanel)
	//*)
END_EVENT_TABLE()

ConfigurationPanel::ConfigurationPanel(wxWindow* parent)
{
	//(*Initialize(ConfigurationPanel)
	wxBoxSizer* execSizer;
	wxBoxSizer* option_sizer;
	wxStaticText* init_cmd_warning;
	wxBoxSizer* main_sizer;
	wxStaticText* exec_path_label;
	wxButton* but_browse;
	wxStaticBoxSizer* init_cmds_sizer;

	Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, _T("wxID_ANY"));
	main_sizer = new wxBoxSizer(wxVERTICAL);
	execSizer = new wxBoxSizer(wxHORIZONTAL);
	exec_path_label = new wxStaticText(this, wxID_ANY, _("Executable path:"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	execSizer->Add(exec_path_label, 0, wxALL|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	m_exec_path = new wxTextCtrl(this, ID_TEXTCTRL_EXEC_PATH, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_TEXTCTRL_EXEC_PATH"));
	execSizer->Add(m_exec_path, 1, wxTOP|wxBOTTOM|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	but_browse = new wxButton(this, ID_BUTTON_BROWSE, _("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT, wxDefaultValidator, _T("ID_BUTTON_BROWSE"));
	execSizer->Add(but_browse, 0, wxALL|wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL, 5);
	main_sizer->Add(execSizer, 0, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 0);
	init_cmds_sizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Initial commands"));
	m_initial_commands = new wxTextCtrl(this, ID_TEXTCTRL_INIT_COMMANDS, wxEmptyString, wxDefaultPosition, wxSize(196,93), wxTE_AUTO_SCROLL|wxTE_MULTILINE|wxTE_DONTWRAP, wxDefaultValidator, _T("ID_TEXTCTRL_INIT_COMMANDS"));
	init_cmds_sizer->Add(m_initial_commands, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 5);
	init_cmd_warning = new wxStaticText(this, wxID_ANY, _("These commands will be sent to the debugger on each session start"), wxDefaultPosition, wxDefaultSize, 0, _T("wxID_ANY"));
	init_cmd_warning->SetForegroundColour(wxColour(128,0,0));
	init_cmds_sizer->Add(init_cmd_warning, 0, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
	main_sizer->Add(init_cmds_sizer, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxEXPAND|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
	option_sizer = new wxBoxSizer(wxVERTICAL);
	m_check_pretty_printers = new wxCheckBox(this, ID_CHECKBOX_PRETTY_PRINTERS, _("Use python pretty printers (gdb >= 7.0 is required)"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX_PRETTY_PRINTERS"));
	m_check_pretty_printers->SetValue(false);
	option_sizer->Add(m_check_pretty_printers, 0, wxTOP|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
	m_check_cpp_excepetions = new wxCheckBox(this, ID_CHECKBOX_CPP_EXCEPTIONS, _("Catch C++ exceptions"), wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator, _T("ID_CHECKBOX_CPP_EXCEPTIONS"));
	m_check_cpp_excepetions->SetValue(false);
	option_sizer->Add(m_check_cpp_excepetions, 0, wxBOTTOM|wxLEFT|wxRIGHT|wxALIGN_LEFT|wxALIGN_BOTTOM, 5);
	main_sizer->Add(option_sizer, 1, wxALL|wxEXPAND|wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL, 0);
	SetSizer(main_sizer);
	main_sizer->Fit(this);
	main_sizer->SetSizeHints(this);
	//*)
}

ConfigurationPanel::~ConfigurationPanel()
{
	//(*Destroy(ConfigurationPanel)
	//*)
}


Configuration::Configuration(const ConfigManagerWrapper &config) : cbDebuggerConfiguration(config)
{
}

cbDebuggerConfiguration* Configuration::Clone() const
{
    return new Configuration(*this);
}

wxPanel* Configuration::MakePanel(wxWindow *parent)
{
    ConfigurationPanel *panel = new ConfigurationPanel(parent);
    panel->m_exec_path->ChangeValue(GetDebuggerExecutable());
    panel->m_initial_commands->SetValue(GetInitialCommandsString());
    panel->m_check_pretty_printers->SetValue(GetFlag(Configuration::PrettyPrinters));
    panel->m_check_cpp_excepetions->SetValue(GetFlag(Configuration::CatchCppExceptions));
    return panel;
}

bool Configuration::SaveChanges(wxPanel *p)
{
    ConfigurationPanel *panel = static_cast<ConfigurationPanel*>(p);

    m_config.Write(wxT("executable_path"), panel->m_exec_path->GetValue());
    m_config.Write(wxT("init_commands"), panel->m_initial_commands->GetValue());
    m_config.Write(wxT("pretty_printer"), panel->m_check_pretty_printers->GetValue());
    m_config.Write(wxT("catch_exceptions"), panel->m_check_cpp_excepetions->GetValue());
    return true;
}

wxString Configuration::GetDebuggerExecutable()
{
    const wxString &result = m_config.Read(wxT("executable_path"), wxEmptyString);
    return !result.empty() ? result : cbDetectDebuggerExecutable(wxT("gdb"));
}

wxArrayString Configuration::GetInitialCommands()
{
    wxArrayString commands;
    wxString const &init = m_config.Read(wxT("init_commands"), wxEmptyString);
    if (!init.empty())
        commands = GetArrayFromString(init, wxT('\n'));
    return commands;
}

wxString Configuration::GetInitialCommandsString()
{
    return m_config.Read(wxT("init_commands"), wxEmptyString);
}

bool Configuration::GetFlag(Flags flag)
{
    switch (flag)
    {
    case PrettyPrinters:
        return m_config.ReadBool(wxT("pretty_printer"), true);
    case CatchCppExceptions:
        return m_config.ReadBool(wxT("catch_exceptions"), false);
    default:
        return false;
    }
}

} // namespace dbg_mi
