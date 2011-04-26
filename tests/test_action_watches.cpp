#include <UnitTest++.h>

#include "actions.h"

#include "common.h"
#include "mock_logger.h"


wxString WatchToString(cbWatch const &watch)
{
    wxString s;
    watch.GetSymbol(s);
    s += wxT("=");

    wxString value;
    watch.GetValue(value);
    s += value;

    if (watch.GetChildCount() > 0)
    {
        s += wxT(" {");
        s += WatchToString(*watch.GetChild(0));

        for (int ii = 1; ii < watch.GetChildCount(); ++ii)
        {
            s += wxT(",");
            s += WatchToString(*watch.GetChild(ii));
        }

        s += wxT("}");
    }

    return s;
}

std::ostream& operator<<(std::ostream &stream, cbWatch const &w)
{
    return stream << WatchToString(w);
}

bool operator == (wxString const &s, cbWatch const &w)
{
    return s == WatchToString(w);
}

dbg_mi::ResultParser MakeParser(wxString const &str)
{
    dbg_mi::ResultParser p;
    if (!p.Parse(str))
        return dbg_mi::ResultParser();
    return p;
}

TEST(UpdateSimple)
{
    dbg_mi::WatchesContainer watches;
    dbg_mi::Watch::Pointer w(new dbg_mi::Watch(wxT("a")));
    w->SetValue(wxT("0"));
    w->SetID(wxT("var1"));
    watches.push_back(w);
    MockLogger logger;
    dbg_mi::WatchesUpdateAction action(watches, logger);
    action.SetID(1);
    action.Start();

    action.OnCommandOutput(dbg_mi::CommandID(1, 0),
                           MakeParser(wxT("^done,changelist=[{name=\"var1\",value=\"5\",")
                                      wxT("in_scope=\"true\",type_changed=\"false\",has_more=\"0\"}]")));

    CHECK_EQUAL(wxT("a=5"), *watches[0]);
    CHECK(watches[0]->IsChanged());
}
