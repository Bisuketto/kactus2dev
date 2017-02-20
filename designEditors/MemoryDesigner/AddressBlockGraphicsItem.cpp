//-----------------------------------------------------------------------------
// File: AddressBlockGraphicsItem.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Mikko Teuho
// Date: 19.09.2016
//
// Description:
// Graphics item for visualizing an address block in the memory designer.
//-----------------------------------------------------------------------------

#include "AddressBlockGraphicsItem.h"

#include <common/KactusColors.h>

#include <designEditors/MemoryDesigner/MemoryItem.h>
#include <designEditors/MemoryDesigner/MemoryMapGraphicsItem.h>
#include <designEditors/MemoryDesigner/RegisterGraphicsItem.h>
#include <designEditors/MemoryDesigner/MemoryConnectionItem.h>
#include <designEditors/MemoryDesigner/MemoryDesignerConstants.h>

#include <QBrush>
#include <QFont>

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::AddressBlockGraphicsItem()
//-----------------------------------------------------------------------------
AddressBlockGraphicsItem::AddressBlockGraphicsItem(QSharedPointer<MemoryItem> blockItem, bool isEmptyBlock,
    bool filterRegisters, bool filterFields, qreal addressBlockWidth, QString const& containingInstanceName,
    MemoryMapGraphicsItem* memoryMapItem):
MemoryDesignerChildGraphicsItem(blockItem->getName(), blockItem->getDisplayName(), QStringLiteral("Address Block"),
    blockItem->getAddress().toULongLong(), blockItem->getRange().toULongLong(), addressBlockWidth,
    containingInstanceName, memoryMapItem),
SubMemoryLayout(blockItem, MemoryDesignerConstants::REGISTER_TYPE, filterRegisters, this),
addressUnitBits_(blockItem->getAUB()),
filterFields_(filterFields)
{
    setColors(KactusColors::ADDR_BLOCK_COLOR, isEmptyBlock);
    setLabelPositions();

    qreal xPosition = MemoryDesignerConstants::MAPSUBITEMPOSITIONX - 1;
    setupSubItems(xPosition, getSubItemType(), blockItem);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::~AddressBlockGraphicsItem()
//-----------------------------------------------------------------------------
AddressBlockGraphicsItem::~AddressBlockGraphicsItem()
{

}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::setLabelPositions()
//-----------------------------------------------------------------------------
void AddressBlockGraphicsItem::setLabelPositions()
{
    qreal nameX = 0;
    QRectF blockRectangle = boundingRect();
    QRectF nameRectangle = getNameLabel()->boundingRect();
    if (subItemsAreFiltered())
    {
        nameX = blockRectangle.right() - nameRectangle.width();
    }
    else
    {
        nameX = blockRectangle.left() + MemoryDesignerConstants::MAPSUBITEMPOSITIONX * 2 - nameRectangle.width();
    }
    qreal nameY = (blockRectangle.height() / 2) - (nameRectangle.height() / 2);

    getNameLabel()->setPos(nameX, nameY);

    qreal rangeStartX = blockRectangle.left();
    qreal rangeStartY = blockRectangle.top();
    qreal rangeEndY = blockRectangle.bottom() - getRangeEndLabel()->boundingRect().height();

    if (!getRangeStartLabel()->isVisible())
    {
        rangeEndY += 2;
    }

    getRangeStartLabel()->setPos(rangeStartX, rangeStartY);
    getRangeEndLabel()->setPos(rangeStartX, rangeEndY);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::createNewSubItem()
//-----------------------------------------------------------------------------
MemoryDesignerChildGraphicsItem* AddressBlockGraphicsItem::createNewSubItem(
    QSharedPointer<MemoryItem> subMemoryItem, bool isEmpty)
{
    int registerWidth = getRegisterWidth();

    return new RegisterGraphicsItem(
        subMemoryItem, isEmpty, registerWidth, filterFields_, getContainingInstance(), this);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::getRegisterWidth()
//-----------------------------------------------------------------------------
qreal AddressBlockGraphicsItem::getRegisterWidth() const
{
    int registerWidth = boundingRect().width() - MemoryDesignerConstants::MAPSUBITEMPOSITIONX * 2 + 1;
    return registerWidth;
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::createEmptySubItem()
//-----------------------------------------------------------------------------
MemoryDesignerChildGraphicsItem* AddressBlockGraphicsItem::createEmptySubItem(quint64 beginAddress,
    quint64 rangeEnd)
{
    quint64 emptyRegisterRangeInt = rangeEnd - beginAddress + 1;

    QString emptyRegisterBaseAddress = QString::number(beginAddress);

    int intAUB = addressUnitBits_.toInt();
    quint64 registerSize = emptyRegisterRangeInt * intAUB;

    QSharedPointer<MemoryItem> emptyRegister (
        new MemoryItem(MemoryDesignerConstants::RESERVED_NAME, MemoryDesignerConstants::REGISTER_TYPE));
    emptyRegister->setAddress(emptyRegisterBaseAddress);
    emptyRegister->setSize(QString::number(registerSize));
    emptyRegister->setAUB(addressUnitBits_);

    return createNewSubItem(emptyRegister, true);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::changeAddressRange()
//-----------------------------------------------------------------------------
void AddressBlockGraphicsItem::changeAddressRange(quint64 memoryMapOffset)
{
    MemoryDesignerChildGraphicsItem::changeAddressRange(memoryMapOffset);

    SubMemoryLayout::changeChildItemRanges(memoryMapOffset);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::addMemoryConnection()
//-----------------------------------------------------------------------------
void AddressBlockGraphicsItem::addMemoryConnection(MemoryConnectionItem* connectionItem)
{
    MemoryDesignerGraphicsItem::addMemoryConnection(connectionItem);

    addConnectionToSubItems(connectionItem);
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::getItemWidth()
//-----------------------------------------------------------------------------
qreal AddressBlockGraphicsItem::getItemWidth() const
{
    qreal registerWidth = 0;

    if (!subItemsAreFiltered())
    {
        registerWidth = getSubMemoryItems().first()->boundingRect().width();
    }
    qreal itemBoundingWidth = boundingRect().width() - registerWidth;
    return itemBoundingWidth;
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::changeWidth()
//-----------------------------------------------------------------------------
void AddressBlockGraphicsItem::changeWidth(qreal widthChange)
{
    if (!subItemsAreFiltered())
    {
        QMapIterator<quint64, MemoryDesignerChildGraphicsItem*> subItemIterator(getSubMemoryItems());
        while (subItemIterator.hasNext())
        {
            subItemIterator.next();
            MemoryDesignerChildGraphicsItem* subItem = subItemIterator.value();
            RegisterGraphicsItem* registerItem = dynamic_cast<RegisterGraphicsItem*>(subItem);
            if (registerItem)
            {
                registerItem->changeWidth(widthChange);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::getMaximumNeededChangeInFieldWidth()
//-----------------------------------------------------------------------------
qreal AddressBlockGraphicsItem::getMaximumNeededChangeInFieldWidth() const
{
    qreal maximumWidthChange = 0;
    if (!filterFields_)
    {
        foreach (MemoryDesignerChildGraphicsItem* subItem, getSubMemoryItems())
        {
            RegisterGraphicsItem* registerItem = dynamic_cast<RegisterGraphicsItem*>(subItem);
            if (registerItem)
            {
                maximumWidthChange = qMax(maximumWidthChange, registerItem->getMaximumNeededChangeInFieldWidth());
            }
        }
    }

    return maximumWidthChange;
}

//-----------------------------------------------------------------------------
// Function: AddressBlockGraphicsItem::createFieldOverlapItems()
//-----------------------------------------------------------------------------
void AddressBlockGraphicsItem::createFieldOverlapItems()
{
    if (!subItemsAreFiltered())
    {
        QMapIterator<quint64, MemoryDesignerChildGraphicsItem*> subItemIterator(getSubMemoryItems());
        while (subItemIterator.hasNext())
        {
            subItemIterator.next();
            MemoryDesignerChildGraphicsItem* subItem = subItemIterator.value();
            RegisterGraphicsItem* registerItem = dynamic_cast<RegisterGraphicsItem*>(subItem);
            if (registerItem)
            {
                registerItem->createOverlappingFieldMarkers();
            }
        }
    }
}
