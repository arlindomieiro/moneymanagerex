/*******************************************************
 Copyright (C) 2006 Madhan Kanagavel

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ********************************************************/

#include "payeedialog.h"
#include "categdialog.h"
#include "relocatepayeedialog.h"
#include "util.h"
#include "mmOption.h"
#include "paths.h"
#include "model/Model_Infotable.h"
#include "model/Model_Payee.h"
#include "model/Model_Category.h"

IMPLEMENT_DYNAMIC_CLASS( mmPayeeDialog, wxDialog )

BEGIN_EVENT_TABLE( mmPayeeDialog, wxDialog )
    EVT_BUTTON(wxID_CANCEL, mmPayeeDialog::OnCancel)
    EVT_BUTTON(wxID_OK, mmPayeeDialog::OnOk)
    EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, mmPayeeDialog::OnListItemSelected)
    EVT_DATAVIEW_ITEM_VALUE_CHANGED(wxID_ANY, mmPayeeDialog::OnDataChanged)
    EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, mmPayeeDialog::OnItemRightClick)
    EVT_MENU_RANGE(MENU_DEFINE_CATEGORY, MENU_RELOCATE_PAYEE, mmPayeeDialog::OnMenuSelected)
END_EVENT_TABLE()


mmPayeeDialog::mmPayeeDialog(wxWindow *parent) :
    m_payee_id_(-1),
    debug_(true),
    refreshRequested_(false)
{
    if (debug_) ColName_[PAYEE_ID]   = _("#");
    ColName_[PAYEE_NAME] = _("Name");
    ColName_[PAYEE_CATEGORY]   = _("Default Category");

    do_create(parent);
}

void mmPayeeDialog::do_create(wxWindow* parent)
{
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);

    long style = wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER;
    if (!wxDialog::Create(parent, wxID_ANY, _("Organize Payees")
        , wxDefaultPosition, wxDefaultSize, style))
    {
        return;
    }

    CreateControls();

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    SetIcon(mmex::getProgramIcon());

    fillControls();

    Centre();
}

void mmPayeeDialog::CreateControls()
{
    int border = 5;
    wxSizerFlags flags, flagsExpand;
    flags.Align(wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL).Border(wxALL, border);
    flagsExpand.Align(wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL).Border(wxALL, border).Expand();

    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer(wxVERTICAL);

    //TODO:provide proper style
    payeeListBox_ = new wxDataViewListCtrl( this
        , wxID_ANY, wxDefaultPosition, wxSize(450, 500)/*, wxDV_HORIZ_RULES*/);

    if (debug_) payeeListBox_ ->AppendTextColumn( ColName_[PAYEE_ID], wxDATAVIEW_CELL_INERT, 30 );
    payeeListBox_ ->AppendTextColumn( ColName_[PAYEE_NAME], wxDATAVIEW_CELL_EDITABLE, 150);
    payeeListBox_ ->AppendTextColumn( ColName_[PAYEE_CATEGORY], wxDATAVIEW_CELL_INERT, 250);
    itemBoxSizer2->Add(payeeListBox_, 1, wxGROW|wxALL, 1);

    wxPanel* buttons_panel = new wxPanel(this, wxID_ANY);
    itemBoxSizer2->Add(buttons_panel, flags.Center().Border(wxALL, 10));

    wxStdDialogButtonSizer*  buttons_sizer = new wxStdDialogButtonSizer;
    buttons_panel->SetSizer(buttons_sizer);

    button_OK_ = new wxButton( buttons_panel, wxID_OK, _("&OK ") );
    btnCancel_ = new wxButton( buttons_panel, wxID_CANCEL, _("&Cancel "));

    buttons_sizer->Add(btnCancel_,  flags);
    buttons_sizer->Add(button_OK_,  flags);
    Center();
    this->SetSizer(itemBoxSizer2);
}

void mmPayeeDialog::fillControls()
{
    Model_Payee::Data_Set filtd = Model_Payee::instance().all(Model_Payee::COL_PAYEENAME);

    payeeListBox_->DeleteAllItems();
    for (const auto& payee: Model_Payee::instance().all(Model_Payee::COL_PAYEENAME))
    {
        const wxString full_category_name = Model_Category::instance().full_name(payee.CATEGID, payee.SUBCATEGID);
        int payeeID = payee.PAYEEID;
        wxVector<wxVariant> data;
        if (debug_) data.push_back(wxVariant(wxString::Format("%i", payeeID)));
        data.push_back(wxVariant(payee.PAYEENAME));
        data.push_back(wxVariant(full_category_name));
        payeeListBox_->AppendItem(data, (wxUIntPtr)payeeID);
    }
}

void mmPayeeDialog::OnDataChanged(wxDataViewEvent& event)
{
    int row = payeeListBox_->ItemToRow(event.GetItem());
    wxVariant var;
    payeeListBox_->GetValue(var, row, event.GetColumn());
    wxString value = var.GetString();
    wxLogDebug(value);

    Model_Payee::Data* payee = Model_Payee::instance().get(m_payee_id_);
    if (payee)
    {
        payee->PAYEENAME = value;
        Model_Payee::instance().save(payee);
    }
    refreshRequested_ = true;

}

void mmPayeeDialog::OnListItemSelected(wxDataViewEvent& event)
{
    wxDataViewItem item = event.GetItem();
    selectedIndex_ = payeeListBox_->ItemToRow(item);
    if (selectedIndex_ < 0) return;

    m_payee_id_ = (int)payeeListBox_->GetItemData(item);
}

void mmPayeeDialog::AddPayee()
{
    wxString text = wxGetTextFromUser(_("Enter the name for the new payee:")
        , _("Organize Payees: Add Payee"), "");
    if (text.IsEmpty()) {
        return;
    }

    Model_Payee::Data* payee = Model_Payee::instance().get(text);
    if (payee)
    {
        wxMessageBox(_("Payee with same name exists"), _("Organize Payees: Add Payee"), wxOK|wxICON_ERROR);
    }
    else
    {
        payee = Model_Payee::instance().create();
        payee->PAYEENAME = text;
        int payeeID = Model_Payee::instance().save(payee);
        if (payeeID < 0) return;
        wxASSERT(payeeID > 0);

        fillControls();
    }
}

void mmPayeeDialog::DeletePayee()
{
    if (!Model_Payee::instance().remove(m_payee_id_))
    {
        wxString deletePayeeErrMsg = _("Payee in use.");
        deletePayeeErrMsg
            << "\n\n"
            << _("Tip: Change all transactions using this Payee to another Payee\nusing the relocate command:")
            << "\n\n" << _("Tools -> Relocation of -> Payees");
        wxMessageBox(deletePayeeErrMsg,_("Organize Payees: Delete Error"), wxOK|wxICON_ERROR);
        return;
    }

    fillControls();
}

void mmPayeeDialog::DefineDefaultCategory()
{
    Model_Payee::Data *payee = Model_Payee::instance().get(m_payee_id_);
    mmCategDialog dlg(this, true, false);
    dlg.setTreeSelection(payee->CATEGID, payee->SUBCATEGID);
    if ( dlg.ShowModal() == wxID_OK )
    {
        payee->CATEGID = dlg.getCategId();
        payee->SUBCATEGID = dlg.getSubCategId();
        refreshRequested_ = true;
        Model_Payee::instance().save(payee);
    }
    else
    {
        payee->CATEGID = -1;
        payee->SUBCATEGID = -1;
        refreshRequested_ = true;
        Model_Payee::instance().save(payee);
    }
    fillControls();
}

void mmPayeeDialog::OnPayeeRelocate()
{
    relocatePayeeDialog dlg(this, m_payee_id_);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxString msgStr;
        msgStr << _("Payee Relocation Completed.") << "\n\n"
            << wxString::Format(_("Records have been updated in the database: %i")
                , dlg.updatedPayeesCount())
            << "\n\n";
        wxMessageBox(msgStr, _("Payee Relocation Result"));
        mmOptions::instance().databaseUpdated_ = true;
        refreshRequested_ = true;
    }
}

void mmPayeeDialog::OnMenuSelected(wxCommandEvent& event)
{
    wxCommandEvent evt;
    int id = event.GetId();
    switch(id)
    {
        case MENU_DEFINE_CATEGORY: DefineDefaultCategory() ; break;
        case NENU_NEW_PAYEE: AddPayee(); break;
        case MENU_DELETE_PAYEE: DeletePayee(); break;
        case MENU_RELOCATE_PAYEE: OnPayeeRelocate(); break;
        default: break;
    }

}

void mmPayeeDialog::OnItemRightClick(wxDataViewEvent& event)
{
    wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_ANY) ;
    evt.SetEventObject( this );

    Model_Payee::Data* payee = Model_Payee::instance().get(m_payee_id_);

    wxMenu* mainMenu = new wxMenu;
    mainMenu->Append(new wxMenuItem(mainMenu, MENU_DEFINE_CATEGORY, _("Define Default Category")));
    if (!payee) mainMenu->Enable(MENU_DEFINE_CATEGORY, false);
    mainMenu->AppendSeparator();

    mainMenu->Append(new wxMenuItem(mainMenu, NENU_NEW_PAYEE, _("&Add ")));
    mainMenu->AppendSeparator();

    mainMenu->Append(new wxMenuItem(mainMenu, MENU_DELETE_PAYEE, _("&Remove ")));
    if (!payee || Model_Payee::is_used(m_payee_id_)) mainMenu->Enable(MENU_DELETE_PAYEE, false);
    mainMenu->AppendSeparator();
    mainMenu->Append(new wxMenuItem(mainMenu, MENU_RELOCATE_PAYEE, _("Relocate Payee")));
    //SetToolTip(_("Change all transactions using one Payee to another Payee"));
    if (!payee) mainMenu->Enable(MENU_RELOCATE_PAYEE, false);

    PopupMenu(mainMenu);
    delete mainMenu;
    event.Skip();
}

void mmPayeeDialog::OnCancel(wxCommandEvent& /*event*/)
{
    EndModal(wxID_CANCEL);
}

void mmPayeeDialog::OnOk(wxCommandEvent& /*event*/)
{
    EndModal(wxID_OK);
}
