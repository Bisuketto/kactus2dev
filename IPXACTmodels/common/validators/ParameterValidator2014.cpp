//-----------------------------------------------------------------------------
// File: ParameterValidator2014.cpp
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Esko Pekkarinen
// Date: 11.12.2014
//
// Description:
// Validator for ipxact:parameter.
//-----------------------------------------------------------------------------

#include "ParameterValidator2014.h"

#include <IPXACTmodels/common/Parameter.h>

#include <IPXACTmodels/Component/choice.h>
#include <IPXACTmodels/common/Enumeration.h>
#include <IPXACTmodels/common/Parameter.h>

#include <IPXACTmodels/common/validators/ValueFormatter.h>

#include <editors/ComponentEditor/common/ExpressionParser.h>

#include <QRegularExpression>
#include <QStringList>

//-----------------------------------------------------------------------------
// Function: SystemVerilogValidator::SystemVerilogValidator()
//-----------------------------------------------------------------------------
ParameterValidator2014::ParameterValidator2014(QSharedPointer<ExpressionParser> expressionParser,
    QSharedPointer<QList<QSharedPointer<Choice> > > availableChoices):
expressionParser_(expressionParser),
availableChoices_(availableChoices)
{

}

//-----------------------------------------------------------------------------
// Function: SystemVerilogValidator::~SystemVerilogValidator()
//-----------------------------------------------------------------------------
ParameterValidator2014::~ParameterValidator2014()
{

}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::validate()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::validate(QSharedPointer<const Parameter> parameter) const
{
    return hasValidName(parameter) &&
        hasValidValue(parameter) &&
        hasValidMinimumValue(parameter) &&
        hasValidMaximumValue(parameter) &&
        hasValidChoice(parameter) &&
        hasValidResolve(parameter) &&
        hasValidValueId(parameter) &&
        hasValidVector(parameter);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidName()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidName(QSharedPointer<const Parameter> parameter) const
{
    QRegularExpression whiteSpaceExpression;
    whiteSpaceExpression.setPattern("^\\s*$");
    QRegularExpressionMatch whiteSpaceMatch = whiteSpaceExpression.match(parameter->name());

    if (parameter->name().isEmpty() || whiteSpaceMatch.hasMatch())
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidValue()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValue(QSharedPointer<const Parameter> parameter) const
{
    return !parameter->getValue().isEmpty() &&
        hasValidValueForType(parameter) &&
        !valueIsLessThanMinimum(parameter) &&
        !valueIsGreaterThanMaximum(parameter) &&
        hasValidValueForChoice(parameter);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidType()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidType(QSharedPointer<const Parameter> parameter) const
{
    QString type = parameter->getType();

    return type.isEmpty() || type == "bit" || type == "byte" || type == "shortint" ||
        type == "int" || type == "longint" || type == "shortreal" || type == "real" || 
        type == "string";
}

//-----------------------------------------------------------------------------
// Function: SystemVerilogValidator::hasValidValueForType()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValueForType(QString const& value, QString const& type) const
{
    if (type.isEmpty())
    {
        return expressionParser_->isValidExpression(value) && expressionParser_->parseExpression(value) != "x";
    }

    if (!expressionParser_->isValidExpression(value))
    {
        return false;
    }

    if (expressionParser_->isArrayExpression(value))
    {
        return isArrayValidForType(value, type);
    }

    bool canConvert = false;
    QString solvedValue = expressionParser_->parseExpression(value);

    if (type == "bit")
    {
        QRegularExpression testi("^[1-9]?[0-9]*'([bB]|[hH])");
        if (testi.match(value).hasMatch())
        {
            ValueFormatter formatter;
            solvedValue = formatter.format(solvedValue, 2);
        }

        QRegularExpression bitExpression("^([01]|[1-9]?[0-9]*'([bB][01_]+|[hH][0-9a-fA-F_]+))$");
        return bitExpression.match(value).hasMatch() || bitExpression.match(solvedValue).hasMatch();
    }
    else if (type == "byte")
    {
        solvedValue.toShort(&canConvert);
        return canConvert && -128 <= solvedValue.toShort() && solvedValue.toShort() <= 127;
    }
    else if (type == "shortint")
    {
        solvedValue.toShort(&canConvert);
    }
    else if (type == "int")
    {
        solvedValue.toInt(&canConvert);
    }
    else if (type == "longint")
    {
        if (solvedValue.startsWith("-"))
        {
            solvedValue.toLongLong(&canConvert);
        }
        else
        {
            solvedValue.toULongLong(&canConvert);
        }
    }
    else if (type == "shortreal")
    {
        solvedValue.toFloat(&canConvert);
    }
    else if (type == "real")
    {
        solvedValue.toDouble(&canConvert);
    }
    else if (type == "string")
    {
        QRegularExpression stringExpression("^\\s*\".*\"\\s*$");
        return stringExpression.match(solvedValue).hasMatch();
    }

    return canConvert;
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::isArrayValidForType()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::isArrayValidForType(QString const& arrayExpression, QString const& type) const
{
    QStringList subValues = splitArrayToList(arrayExpression);

    if (arrayValuesAreSameSize(subValues, type))
    {
        foreach (QString innerValue, subValues)
        {
            innerValue = innerValue.remove(" ");
            if (!hasValidValueForType(innerValue, type))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidVector()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidVector(QSharedPointer<const Parameter> parameter) const
{
    if (!parameter->getVectors()->isEmpty() && parameter->getType() != QLatin1String("bit"))
    {
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidValueForFormat()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValueForType(QSharedPointer<const Parameter> parameter) const
{
    return hasValidValueForType(parameter->getValue(), parameter->getType());
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidMinimumValue()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidMinimumValue(QSharedPointer<const Parameter> parameter) const
{
    QString minimumValue = parameter->getMinimumValue();

    if (!shouldCompareValueAndBoundary(minimumValue, parameter->getType()))
    {
        return true;
    }

    return hasValidValueForType(minimumValue, parameter->getType());
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidMaximumValue()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidMaximumValue(QSharedPointer<const Parameter> parameter) const
{
    QString maximumValue = parameter->getMaximumValue();

    if (!shouldCompareValueAndBoundary(maximumValue, parameter->getType()))
    {
        return true;
    }

    return hasValidValueForType(maximumValue, parameter->getType());
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidChoice()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidChoice(QSharedPointer<const Parameter> parameter) const
{
    return parameter->getChoiceRef().isEmpty() || findChoiceByName(parameter->getChoiceRef());
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidValueForChoice()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValueForChoice(QSharedPointer<const Parameter> parameter) const
{
    if (parameter->getChoiceRef().isEmpty())
    {
        return true;
    }
    else
    {
        QSharedPointer<Choice> referencedChoice = findChoiceByName(parameter->getChoiceRef());

        if (!referencedChoice.isNull() && parameter->getValue().contains('{') &&
            parameter->getValue().contains('}'))
        {
            QStringList valueArray = parameter->getValue().split(',');
            valueArray.first().remove('{');
            valueArray.last().remove('}');

            foreach (QString parameterValue, valueArray)
            {
                if (!referencedChoice->hasEnumeration(parameterValue))
                {
                    return false;
                }
            }
            return true;
        }

        return !referencedChoice.isNull() && referencedChoice->hasEnumeration(parameter->getValue());
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidResolve()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidResolve(QSharedPointer<const Parameter> parameter) const
{
    QString resolve = parameter->getValueResolve();

    return resolve.isEmpty() || resolve == "immediate" || resolve == "user" || resolve == "generated";
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidValueId()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValueId(QSharedPointer<const Parameter> parameter) const
{
    QString resolve = parameter->getValueResolve();

    if (resolve == "user" || resolve == "generated")
    {
        return !parameter->getValueId().isEmpty();
    }

    return true;
}


//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::valueIsLessThanMinimum()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::valueIsLessThanMinimum(QSharedPointer<const Parameter> parameter) const
{
    QString minimum = parameter->getMinimumValue();
    QString type = parameter->getType();
    QString value = expressionParser_->parseExpression(parameter->getValue());

    if (expressionParser_->isArrayExpression(value) && type != "bit" && type != "string" && !type.isEmpty())
    {
        QStringList subValues = splitArrayToList(value);

        foreach (QString innerValue, subValues)
        {
            if (shouldCompareValueAndBoundary(minimum, type) && valueOf(innerValue, type) < valueOf(minimum, type))
            {
                return true;
            }
        }

        return false;
    }

    return shouldCompareValueAndBoundary(minimum, type) && valueOf(value, type) < valueOf(minimum, type);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::valueIsGreaterThanMaximum()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::valueIsGreaterThanMaximum(QSharedPointer<const Parameter> parameter) const
{
    QString maximum = parameter->getMaximumValue();
    QString type = parameter->getType();
    QString value = expressionParser_->parseExpression(parameter->getValue());

    if (expressionParser_->isArrayExpression(value) && type != "bit" && type != "string" && !type.isEmpty())
    {
        QStringList subValues = splitArrayToList(value);

        foreach (QString innerValue, subValues)
        {
            if (shouldCompareValueAndBoundary(maximum, type) && valueOf(innerValue, type) > valueOf(maximum, type))
            {
                return true;
            }
        }

        return false;
    }

    return shouldCompareValueAndBoundary(maximum, type) && valueOf(value, type) > valueOf(maximum, type);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsIn()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsIn(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    findErrorsInName(errors, parameter, context);
    findErrorsInValue(errors, parameter, context);
    findErrorsInType(errors, parameter, context);
    findErrorsInMinimumValue(errors, parameter, context);
    findErrorsInMaximumValue(errors, parameter, context);
    findErrorsInChoice(errors, parameter, context);
    findErrorsInResolve(errors, parameter, context);
    findErrorsInVector(errors, parameter, context);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::shouldCompareValueAndBoundary()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::shouldCompareValueAndBoundary(QString const& boundaryValue, QString const& type) const
{
     return expressionParser_->isValidExpression(boundaryValue) && 
         (type != "bit" && type != "string" && !type.isEmpty());
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::valueOf()
//-----------------------------------------------------------------------------
qreal ParameterValidator2014::valueOf(QString const& value, QString const& type) const
{
    if (type == "real" || type == "shortreal")
    {
        return expressionParser_->parseExpression(value).toDouble();
    }
    else
    {
        return expressionParser_->parseExpression(value).toLongLong();
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInName()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInName(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (!hasValidName(parameter))
    {
        errors.append(QObject::tr("No valid name specified for %1 %2 within %3").arg(parameter->elementName(),
            parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInType()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInType(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (!hasValidType(parameter))
    {
        errors.append(QObject::tr("Invalid type %1 specified for %2 %3 within %4").arg(
            parameter->getType(), parameter->elementName(), parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInValue()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInValue(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (parameter->getValue().isEmpty())
    {
        errors.append(QObject::tr("No value specified for %1 %2 within %3").arg(
            parameter->elementName(), parameter->name(), context));
    }
    else
    {
        if (!hasValidValueForType(parameter))
        {
            errors.append(QObject::tr("Value %1 is not valid for type %2 in %3 %4 within %5").arg(
                parameter->getValue(), parameter->getType(), parameter->elementName(), 
                parameter->name(), context));
        }

        if (valueIsLessThanMinimum(parameter))
        {
            errors.append(QObject::tr("Value %1 violates minimum value %2 in %3 %4 within %5"
                ).arg(parameter->getValue(), parameter->getMinimumValue(), 
                parameter->elementName(), parameter->name(), context));
        }

        if (valueIsGreaterThanMaximum(parameter))
        {
            errors.append(QObject::tr("Value %1 violates maximum value %2 in %3 %4 within %5"
                ).arg(parameter->getValue(), parameter->getMaximumValue(), 
                parameter->elementName(), parameter->name(), context));
        }

        if (!hasValidValueForChoice(parameter))
        {           
            errors.append(QObject::tr("Value %1 references unknown enumeration for choice "
                "%2 in %3 %4 within %5").arg(parameter->getValue(), parameter->getChoiceRef(), 
                parameter->elementName(), parameter->name(), context));
        }
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInMinimumValue()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInMinimumValue(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (shouldCompareValueAndBoundary(parameter->getValueAttribute("ipxact:minimum"), parameter->getType())
        && !hasValidValueForFormat(parameter->getValueAttribute("ipxact:minimum")))
    {
        errors.append(QObject::tr("Minimum value %1 is not valid for format %2 in %3 %4 within %5").arg(
            parameter->getValueAttribute("ipxact:minimum"), parameter->getValueAttribute("ipxact:format"),parameter->elementName(),
            parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInMaximumValue()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInMaximumValue(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (shouldCompareValueAndBoundary(parameter->getValueAttribute("ipxact:maximum"), parameter->getType())
        && !hasValidValueForFormat(parameter->getValueAttribute("ipxact:maximum")))
    {
        errors.append(QObject::tr("Maximum value %1 is not valid for format %2 in %3 %4 within %5").arg(
            parameter->getValueAttribute("ipxact:maximum"),
            parameter->getValueAttribute("ipxact:format"),parameter->elementName(), parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator::isValidValueForFormat()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::hasValidValueForFormat(QString const& value) const
{
    return expressionParser_->isValidExpression(value);
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInChoice()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInChoice(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (!hasValidChoice(parameter))
    { 
        errors.append(QObject::tr("Choice %1 referenced in %2 %3 is not specified within %4").arg(
            parameter->getChoiceRef(), parameter->elementName(), parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInResolve()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInResolve(QVector<QString>& errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (!hasValidResolve(parameter))
    { 
        errors.append(QObject::tr("Invalid resolve %1 specified for %2 %3 within %4").arg(
            parameter->getValueResolve(), parameter->elementName(), parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::findErrorsInVector()
//-----------------------------------------------------------------------------
void ParameterValidator2014::findErrorsInVector(QVector<QString> errors, QSharedPointer<Parameter> parameter,
    QString const& context) const
{
    if (!hasValidVector(parameter))
    {
        errors.append(QObject::tr("Invalid vector specified for %1 %2 within %3").arg(parameter->elementName(),
            parameter->name(), context));
    }
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::splitArrayToList()
//-----------------------------------------------------------------------------
QStringList ParameterValidator2014::splitArrayToList(QString const& arrayValue) const
{
    QStringList subValues = arrayValue.split(',');
    QRegularExpression startExpression ("^'?{");

    subValues.first().remove(startExpression);
    subValues.last().chop(1);

    return subValues;
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::arrayValuesAreSameSize()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::arrayValuesAreSameSize(QStringList const& bitArray, QString type) const
{
    if (type == "bit" && bitArray.size() > 1)
    {
        ValueFormatter formatter;
        QString formattedFirst = formatter.format(expressionParser_->parseExpression(bitArray.first()), 2);
        int bitSize = formattedFirst.size();

        for (int i = 1; i < bitArray.size(); ++i)
        {
            QString solvedValue = expressionParser_->parseExpression(bitArray.at(i));
            QString formattedValue = formatter.format(solvedValue, 2);

            if (bitSize != formattedValue.size())
            {
                return false;
            }
        }
    }

    return true;
}

//-----------------------------------------------------------------------------
// Function: ParameterValidator2014::hasValidArrayValues()
//-----------------------------------------------------------------------------
bool ParameterValidator2014::validateArrayValues(QString const& arrayLeft, QString const& arrayRight) const
{
    if (arrayLeft.isEmpty() && arrayRight.isEmpty())
    {
        return true;
    }

    else
    {
        bool arrayLeftIsOk = true;
        bool arrayRightIsOk = true;

        QString leftValue = expressionParser_->parseExpression(arrayLeft);
        QString rightValue = expressionParser_->parseExpression(arrayRight);
        leftValue.toInt(&arrayLeftIsOk);
        rightValue.toInt(&arrayRightIsOk);

        return arrayLeftIsOk && arrayRightIsOk;
    }
}

//-----------------------------------------------------------------------------
// Function: AbstractParameterModel::findChoiceByName()
//-----------------------------------------------------------------------------
QSharedPointer<Choice> ParameterValidator2014::findChoiceByName(QString const& choiceName) const
{
    if (availableChoices_)
    {
        foreach (QSharedPointer<Choice> choice, *availableChoices_)
        {
            if (choice->name() == choiceName)
            {
                return choice;
            }
        }	
    }

    return QSharedPointer<Choice>();
}
