/*
 * Copyright 2011-2012 Benoit Averty, Samuel Babin, Matthieu Bergere, Thomas Letan, Sacha Percot-Tétu, Florian Teyssier
 * 
 * This file is part of DETIQ-T.
 * 
 * DETIQ-T is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * DETIQ-T is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with DETIQ-T.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "FilterChoice.h"

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QTableView>

#include <QFile>
#include <QtXml/QDomDocument>
#include <QtXml/QDomImplementation>
#include <QtXml/QDomElement>
#include <QTextStream>

#include <QFormLayout>
#include <QSpacerItem>
#include <GenericInterface.h>

using namespace filtrme;
using namespace genericinterface;
using namespace imagein;
using namespace algorithm;

FilterChoice::FilterChoice(QWidget* parent) : QDialog(parent)
{
  initUI();
}

void FilterChoice::initUI()
{

    this->setWindowTitle(tr("FilterChoice"));
    QLayout* layout = new QVBoxLayout(this);
    QWidget* mainWidget = new QWidget();
    layout->addWidget(mainWidget);
    QHBoxLayout* mainLayout = new QHBoxLayout(mainWidget);
    QWidget* leftWidget = new QWidget();
    QFormLayout* leftLayout = new QFormLayout(leftWidget);

    /* FILTER CHOICE */
    QLabel* label = new QLabel(this);
    label->setText(tr("Filter:"));
    _blurChoices = new QComboBox(this);
    QStringList blurs = initFilters();
    _blurChoices->addItems(blurs);
    QObject::connect(_blurChoices, SIGNAL(currentIndexChanged(int)), this, SLOT(currentBlurChanged(int)));
    leftLayout->addRow(label, _blurChoices);

    /* POLICIES CHOICE */
    QLabel* label_2 = new QLabel(this);
    label_2->setText(tr("Edge policy: "));
    _policyChoices = new QComboBox(this);
    QStringList policies = QStringList() << tr("Black") << tr("Mirror") << tr("Nearest") << tr("Spherical");
    _policyChoices->addItems(policies);
    leftLayout->addRow(label_2, _policyChoices);

    _labelNumber = new QLabel(this);
    _labelNumber->setText(tr("Number of pixels:"));
    _number = new QSpinBox(this);
    _number->setValue(3);
    _number->setMinimum(1);
    leftLayout->addRow(_labelNumber, _number);

    mainLayout->addWidget(leftWidget);

    QObject::connect(_number, SIGNAL(valueChanged(const QString&)), this, SLOT(dataChanged(const QString&)));


    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);

    /* INIT FILTER */
    _filterView = new QTableWidget(3, 3, this);
    _filterView->verticalHeader()->hide();
    _filterView->horizontalHeader()->hide();
    _filterView->setDragEnabled(false);
    _filterView->setCornerButtonEnabled(false);

    int numPixels = _number->value();
    for(int i = 0; i < numPixels; i++)
    {
      _filterView->setColumnWidth(i, _filterView->rowHeight(0));
      for(int j = 0; j < numPixels; j++)
      {
        QTableWidgetItem* item = new QTableWidgetItem("1");
        item->setTextAlignment(Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        _filterView->setItem(i, j, item);
      }
    }

    rightLayout->addWidget(_filterView);
  
    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    QPushButton* applyButton = buttonBox->addButton(tr("Apply filter"), QDialogButtonBox::ApplyRole);
    _deleteButton = buttonBox->addButton(tr("Delete filter"), QDialogButtonBox::ActionRole);
    _deleteButton->setEnabled(false);

    QObject::connect(applyButton, SIGNAL(clicked()), this, SLOT(validate()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QObject::connect(_deleteButton, SIGNAL(clicked()), this, SLOT(deleteFilter()));

//    QLayoutItem* spaceItem = new QSpacerItem(512, 512);
//    leftLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
//    leftLayout->setItem(2, QFormLayout::SpanningRole, spaceItem);
//    QWidget* spacerWidget = new QWidget();
//    spacerWidget->setFixedSize(32, 32);
//    spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
//    leftWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
//    leftLayout->addRow(spacerWidget);
//    leftLayout->addRow(buttonBox);

    mainLayout->addWidget(rightWidget);
    layout->addWidget(buttonBox);

}

QStringList FilterChoice::initFilters() {

  QStringList blurs = QStringList();
  blurs << tr("Uniform") << tr("Gaussian") << tr("Prewitt") << tr("Roberts") << tr("Sobel") << tr("SquareLaplacien");

  _filters.push_back(Filter::uniform(3));
  _filters.push_back(Filter::gaussian(1));
  _filters.push_back(Filter::prewitt(3));
  _filters.push_back(Filter::roberts());
  _filters.push_back(Filter::sobel());
  _filters.push_back(Filter::squareLaplacien());

  //Personal filters
  QFile file("filters.xml");
  if(file.exists())
  {
    QDomDocument doc("");
    file.open(QIODevice::ReadOnly);
      doc.setContent(&file);
      file.close();

    QDomElement root = doc.documentElement();
    QDomNode child = root.firstChild();
      while(!child.isNull())
      {
      QDomElement e = child.toElement();
        // We know how to treat appearance and geometry
      blurs.push_back(e.attribute("name"));

      int nbFilters = e.attribute("nbFilters").toInt();

      std::vector<Filter*> temp;
      QDomNode grandChild = e.firstChild();
      for(int i = 0; i < nbFilters && !grandChild.isNull(); i++)
      {
        QDomElement grandChildElement = grandChild.toElement();
        if(!grandChildElement.isNull())
        {
          Filter* f = new Filter(grandChildElement.attribute("width").toInt(), grandChildElement.attribute("height").toInt());
          // We know how to treat color
          if (grandChildElement.tagName() == "values")
          {
            std::string str = grandChildElement.text().toStdString();
            std::string word;
            std::stringstream stream(str);
            unsigned int w = 0, h = 0;
            while(getline(stream, word, ' '))
            {
//              (*f)[w][h] = QString::fromStdString(word).toInt();
                f->setPixelAt(w, h, QString::fromStdString(word).toDouble());

              if(h == f->getHeight() - 1)
              {
                h = 0;
                w++;
              }
              else
                h++;
            }
          }
          temp.push_back(f);
        }
        grandChild = grandChild.nextSibling();
      }
      _filters.push_back(temp);

      child = child.nextSibling();
      }
  }
  return blurs;
}

void FilterChoice::currentBlurChanged(int)
{
  updateDisplay();
}

void FilterChoice::dataChanged(const QString&)
{
  updateDisplay();
}

void FilterChoice::validate()
{
  int num = _number->value();
  
  switch(_blurChoices->currentIndex())
  {
    case 0:
      _filtering = new Filtering(Filtering::uniformBlur(num));
      break;
    case 1:
      _filtering = new Filtering(Filtering::gaussianBlur(num));
      break;
    case 2:
      _filtering = new Filtering(Filtering::prewitt(num));
      break;
    default:
      _filtering = new Filtering(_filters[_blurChoices->currentIndex()]);
  }
  
  switch(_policyChoices->currentIndex())
  {
    case 0:
      _filtering->setPolicy(Filtering::blackPolicy);
      break;
    case 1:
      _filtering->setPolicy(Filtering::mirrorPolicy);
      break;
    case 2:
      _filtering->setPolicy(Filtering::nearestPolicy);
      break;
    case 3:
      _filtering->setPolicy(Filtering::sphericalPolicy);
      break;
    default:
      _filtering->setPolicy(Filtering::blackPolicy);
  }
  this->accept();
}

void FilterChoice::cancel()
{
//  emit(cancelAction());
}

void FilterChoice::deleteFilter()
{
  QMessageBox msgBox(QMessageBox::Warning, tr("Warning!"), tr("This filter will be permanently deleted ?"));
  msgBox.setInformativeText(tr("Do you want to continue?"));
  msgBox.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  msgBox.setDefaultButton(QMessageBox::No);
  
  if(msgBox.exec() == QMessageBox::Yes)
  {
    QString name = _blurChoices->itemText(_blurChoices->currentIndex());
    _blurChoices->setCurrentIndex(_blurChoices->currentIndex() - 1);
    _blurChoices->removeItem(_blurChoices->currentIndex() + 1);
    QFile file("filters.xml");
    if(file.exists())
    {
      QDomDocument doc("");
      file.open(QIODevice::ReadOnly);
      doc.setContent(&file);
      file.close();
      
      QDomElement root = doc.documentElement();
      QDomNode child = root.firstChild();
      while(!child.isNull())
      {
        QDomElement e = child.toElement();
        // We know how to treat appearance and geometry
        if (e.attribute("name") == name)
        {
          doc.documentElement().removeChild(child);
          break;
        }
        child = child.nextSibling();
      }
      
      if(file.open(QFile::WriteOnly))
      {
        QTextStream out(&file);
        out << doc.toString();
        file.close();
      }
    }
  }
}

void FilterChoice::updateDisplay()
{
  std::vector<Filter*> filters;
  int num = _number->value();
  switch(_blurChoices->currentIndex())
  {
    case 0:
      filters = Filter::uniform(num);
      _number->show();
      _labelNumber->show();
      _labelNumber->setText(tr("Number of Pixels:"));
      break;
    case 1:
      filters = Filter::gaussian(num);
      _number->show();
      _labelNumber->show();
      _labelNumber->setText(tr("Coefficient:"));
      break;
    case 2:
      filters = Filter::prewitt(num);
      _number->show();
      _labelNumber->show();
      _labelNumber->setText(tr("Number of Pixels:"));
      break;
    default:
      filters = _filters[_blurChoices->currentIndex()];
      _number->hide();
      _labelNumber->hide();
  }
  
  if(_blurChoices->currentIndex() > 5)
    _deleteButton->setEnabled(true);
  else
    _deleteButton->setEnabled(false);
  
  unsigned int height(0), width(0);
  
  for(unsigned int i = 0; i < filters.size(); i++)
  {
    if(height > 0)
      height++;
    height += filters[i]->getHeight();
    if(filters[i]->getWidth() > width)
      width = filters[i]->getWidth();
  }
  _filterView->setRowCount(height);
  _filterView->setColumnCount(width);
  for(unsigned int i = 0; i < height; i++)
  {
    for(unsigned int j = 0; j < width; j++)
    {
      QTableWidgetItem* item = new QTableWidgetItem("");
      item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      _filterView->setItem(i, j, item);
    }
  }
  
  height = 0;
  for(unsigned int i = 0; i < filters.size(); i++)
  {
    for(unsigned int j = height; j < filters[i]->getWidth() + height; j++)
    {
      for(unsigned int k = 0; k < filters[i]->getHeight(); k++)
      {
//        int value = (*filters[i])[j - height][k];
        double value = filters[i]->getPixelAt(j - height, k);
        QTableWidgetItem* item = new QTableWidgetItem(QString::number(value));
        item->setTextAlignment(Qt::AlignHCenter);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        _filterView->setItem(j, k, item);
        _filterView->setColumnWidth(k, _filterView->rowHeight(0));
      }
    }
    
    height += filters[i]->getWidth();
    for(unsigned int k = 0; k < filters[i]->getHeight(); k++)
    {
      QTableWidgetItem* item = new QTableWidgetItem("");
      item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
      _filterView->setItem(height, k, item);
    }
    
    height++;
  }
  
  _filterView->resizeColumnsToContents();
  _filterView->resizeRowsToContents();
}
