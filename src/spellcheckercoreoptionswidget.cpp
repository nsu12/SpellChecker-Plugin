/**************************************************************************
**
** Copyright (c) 2014 Carel Combrink
**
** This file is part of the SpellChecker Plugin, a Qt Creator plugin.
**
** The SpellChecker Plugin is free software: you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public License as
** published by the Free Software Foundation, either version 3 of the
** License, or (at your option) any later version.
**
** The SpellChecker Plugin is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with the SpellChecker Plugin.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include "ISpellChecker.h"
#include "spellcheckercore.h"
#include "spellcheckercoreoptionswidget.h"
#include "spellcheckercoresettings.h"
#include "ui_spellcheckercoreoptionswidget.h"

#include <utils/utilsicons.h>

using namespace SpellChecker::Internal;

SpellCheckerCoreOptionsWidget::SpellCheckerCoreOptionsWidget( const SpellChecker::Internal::SpellCheckerCoreSettings* const settings, QWidget* parent )
  : QWidget( parent )
  , ui( new Ui::SpellCheckerCoreOptionsWidget )
  , m_currentCheckerOptionsWidget( nullptr )
{
  ui->setupUi( this );
  connect( ui->comboBoxSpellChecker,    &QComboBox::currentTextChanged, this, &SpellCheckerCoreOptionsWidget::comboBoxSpellCheckerCurrentTextChanged );
  connect( ui->toolButtonAddProject,    &QToolButton::clicked,          this, &SpellCheckerCoreOptionsWidget::toolButtonAddProjectClicked );
  connect( ui->toolButtonRemoveProject, &QToolButton::clicked,          this, &SpellCheckerCoreOptionsWidget::toolButtonRemoveProjectClicked );

  /* Hide the error widget by default, since there should not be errors. */
  ui->widgetErrorOutput->setVisible( false );
  ui->toolButtonAddProject->setIcon( Utils::Icons::PLUS.icon() );
  /* Manually create the correct Minus icon in the same way that the Icon
   * Utils creates the PLUS icon. The reason for this is that the PLUS
   * icon also has a PLUS_TOOLBAR icon for toolbars, but the Minus icon
   * does not have these two icons, it only has the one, and that one
   * is for the toolbar since nowhere else is a normal one needed. */
  ui->toolButtonRemoveProject->setIcon( Utils::Icon( { { ":/utils/images/minus.png"
                                                       , Utils::Theme::PaletteText } }, Utils::Icon::Tint ).icon() );

  QMap<QString, ISpellChecker*> availableSpellCheckers = SpellCheckerCore::instance()->addedSpellCheckers();
  for( const QString& name: availableSpellCheckers.keys() ) {
    ui->comboBoxSpellChecker->addItem( name );
  }

  /* Logic to select the current checker in the list of available checkers.
   * The core is implemented in such a way that if one is added and there is
   * none set, the newly added one will be set as the default checker.
   * This logic will ensure that the correct one is selected. */
  int index                          = -1;
  ISpellChecker* currentSpellChecker = SpellCheckerCore::instance()->spellChecker();
  if( currentSpellChecker != nullptr ) {
    index = ui->comboBoxSpellChecker->findText( currentSpellChecker->name() );
  } else {
    qDebug() << "SpellChecker Plugin: No active spellcheckers...";
  }

  ui->comboBoxSpellChecker->setCurrentIndex( index );

  updateWithSettings( settings );
}
// --------------------------------------------------

SpellCheckerCoreOptionsWidget::~SpellCheckerCoreOptionsWidget()
{
  delete ui;
}
// --------------------------------------------------

const SpellCheckerCoreSettings& SpellCheckerCoreOptionsWidget::settings()
{
  m_settings.activeSpellChecker       = ui->comboBoxSpellChecker->currentText();
  m_settings.onlyParseCurrentFile     = ui->checkBoxOnlyCheckCurrent->isChecked();
  m_settings.checkExternalFiles       = ui->checkBoxCheckExternal->isChecked();
  m_settings.projectsToIgnore         = m_projectsToIgnore;
  m_settings.replaceAllFromRightClick = ui->checkBoxReplaceAllRightClick->isChecked();
  return m_settings;
}
// --------------------------------------------------

void SpellCheckerCoreOptionsWidget::applySettings()
{
  ui->labelError->clear();
  ui->widgetErrorOutput->setVisible( false );
  emit applyCurrentSetSettings();
}
// --------------------------------------------------

void SpellCheckerCoreOptionsWidget::optionsPageError( const QString& optionsPage, const QString& error )
{
  QString displayString = QString( QLatin1String( "<b>%1</b>: %2" ) ).arg( optionsPage, error );
  ui->labelError->setText( displayString );
  ui->widgetErrorOutput->setVisible( true );
}
// --------------------------------------------------

void SpellCheckerCoreOptionsWidget::updateWithSettings( const SpellCheckerCoreSettings* const settings )
{
  int index;
  if( ( settings->activeSpellChecker.isEmpty() == true )
      && ( ui->comboBoxSpellChecker->count() > 0 ) ) {
    index = 0;
  } else {
    index = ui->comboBoxSpellChecker->findText( settings->activeSpellChecker );
    if( index == -1 ) {
      qDebug() << "Spellchecker from settings not valid option: " << settings->activeSpellChecker;
      Q_ASSERT_X( false, "updateWithSettings", ("Spellchecker from settings not valid option: " + settings->activeSpellChecker).toLatin1() );
      return;
    }
  }
  ui->comboBoxSpellChecker->setCurrentIndex( index );
  ui->checkBoxOnlyCheckCurrent->setChecked( settings->onlyParseCurrentFile );
  ui->checkBoxCheckExternal->setChecked( settings->checkExternalFiles );
  m_projectsToIgnore = settings->projectsToIgnore;
  m_projectsToIgnore.removeDuplicates();
  ui->listWidget->clear();
  ui->listWidget->addItems( m_projectsToIgnore );
  ui->checkBoxReplaceAllRightClick->setChecked( settings->replaceAllFromRightClick );
}
// --------------------------------------------------

void SpellChecker::Internal::SpellCheckerCoreOptionsWidget::comboBoxSpellCheckerCurrentTextChanged( const QString& arg1 )
{
  if( m_currentCheckerOptionsWidget != nullptr ) {
    delete m_currentCheckerOptionsWidget;
    m_currentCheckerOptionsWidget = nullptr;
  }

  if( arg1.isEmpty() == true ) {
    return;
  }

  QMap<QString, ISpellChecker*> availableSpellCheckers = SpellCheckerCore::instance()->addedSpellCheckers();
  if( availableSpellCheckers.contains( arg1 ) == false ) {
    return;
  }
  QWidget* widget = availableSpellCheckers[arg1]->optionsWidget();
  ui->spellCheckerOptionsWidgetLayout->addWidget( widget );
  connect( this,   SIGNAL( applyCurrentSetSettings() ),   widget, SLOT( applySettings() ) );
  connect( widget, SIGNAL(optionsError(QString,QString)), this,   SLOT( optionsPageError( QString, QString ) ) );
  m_currentCheckerOptionsWidget = widget;
}
// --------------------------------------------------

void SpellChecker::Internal::SpellCheckerCoreOptionsWidget::toolButtonAddProjectClicked()
{
  QListWidgetItem* item = new QListWidgetItem();
  item->setData( Qt::DisplayPropertyRole, QString() );
  item->setFlags( item->flags() | Qt::ItemIsEditable );
  ui->listWidget->addItem( item );

  ui->listWidget->setCurrentItem( item );
  ui->listWidget->editItem( item );

  connect( ui->listWidget, &QListWidget::itemChanged, this, &SpellCheckerCoreOptionsWidget::listWidgetItemChanged );
}
// --------------------------------------------------

void SpellChecker::Internal::SpellCheckerCoreOptionsWidget::toolButtonRemoveProjectClicked()
{
  int row = ui->listWidget->currentRow();
  if( row == -1 ) {
    return;
  }
  QListWidgetItem* item = ui->listWidget->takeItem( row );
  QString projectName   = item->data( Qt::DisplayRole ).toString();
  m_projectsToIgnore.removeAll( projectName );
  delete item;
}
// --------------------------------------------------

void SpellChecker::Internal::SpellCheckerCoreOptionsWidget::listWidgetItemChanged( QListWidgetItem* item )
{
  disconnect( ui->listWidget, &QListWidget::itemChanged, this, &SpellCheckerCoreOptionsWidget::listWidgetItemChanged );
  if( item == nullptr ) {
    return;
  }
  QString newProjectName = item->data( Qt::EditRole ).toString();
  item->setFlags( item->flags() & ( ~Qt::ItemIsEditable ) );
  if( ( newProjectName.isEmpty() == true )
      || ( m_projectsToIgnore.contains( newProjectName ) == true ) ) {
    /* Remove it */
    toolButtonRemoveProjectClicked();
    optionsPageError( QStringLiteral( "SpellChecker" )
                      , ( newProjectName.isEmpty() == true )
                      ? QStringLiteral( "Project name can not be empty" )
                      : QStringLiteral( "Project already exists in the list of projects to ignore" ) );
  } else {
    /* Add it */
    m_projectsToIgnore << newProjectName;
  }
}
// --------------------------------------------------
