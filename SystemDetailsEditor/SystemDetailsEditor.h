//-----------------------------------------------------------------------------
// File: SystemDetailsEditor.h
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Joni-Matti M��tt�
// Date: 30.7.2012
//
// Description:
// Docking editor for system details.
//-----------------------------------------------------------------------------

#ifndef SYSTEMDETAILSEDITOR_H
#define SYSTEMDETAILSEDITOR_H

#include <common/widgets/vlnvEditor/vlnveditor.h>

#include <models/component.h>

#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QSharedPointer>
#include <QList>

class DesignWidget;
class SystemView;
class LibraryInterface;

//-----------------------------------------------------------------------------
//! Docking editor to edit system details.
//-----------------------------------------------------------------------------
class SystemDetailsEditor : public QWidget
{
	Q_OBJECT

public:
	/*!
     *  Constructor.
	 *
	 *      @param [in] handler The library interface.
	 *      @param [in] parent  The parent widget.
	 */
	SystemDetailsEditor(LibraryInterface* handler, QWidget *parent);
	
	/*!
     *  Destructor.
     */
	virtual ~SystemDetailsEditor();

	/*!
     *  Set the system to be displayed.
	 *
	 *      @param [in] designWidget The system design widget containing the opened system.
	 *      @param [in] locked       If true, the system details are locked and cannot be modified.
	 */
	void setSystem(DesignWidget* designWidget, bool locked);

	/*!
     *  Clear the contents of this editor.
	 */
	void clear();

public slots:
	//! Handler for design widget's refreshed signal.
	void onRefresh();

	//! Set the editor to locked/unlocked mode.
	void setLocked(bool locked);

signals:
	//! Emitted when the editor changes some value.
	void contentChanged();

private slots:
    //! Called when the view reference has been changed from the combo box.
    void onViewRefChanged(QString const& viewRef);

private:
    // Disable copying.
    SystemDetailsEditor(SystemDetailsEditor const& rhs);
    SystemDetailsEditor& operator=(SystemDetailsEditor const& rhs);

	/*! 
     *  Sets up the layout of this widget.
     */
	void setupLayout();

	/*!
     *  Set up the signals between widgets.
     */
	void setupConnections();

    //-----------------------------------------------------------------------------
    // Data.
    //-----------------------------------------------------------------------------

	//! Pointer to the instance that manages the library.
	LibraryInterface* handler_;

    //! HW reference editor.
    VLNVEditor hwRefEditor_;

	//! Combo box to select the used configuration.
	QComboBox viewSelector_;

    //! Button to remove mappings.
    QPushButton removeMappingButton_;

	//! Pointer to the top component being edited.
	QSharedPointer<Component> component_;

	//! Pointer to the design widget that contains the design.
	DesignWidget* designWidget_;

    //! The system view containing the edited system.
    SystemView* systemView_;
};

//-----------------------------------------------------------------------------

#endif // SYSTEMDETAILSEDITOR_H