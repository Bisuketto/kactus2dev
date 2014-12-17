//-----------------------------------------------------------------------------
// File: tst_ParameterValidator2014.cpp
//-----------------------------------------------------------------------------
// Project: Kactus 2
// Author: Esko Pekkarinen
// Date: 11.12.2014
//
// Description:
// Unit test for class SystemVerilogValidator.
//-----------------------------------------------------------------------------

#include <QtTest>

#include <editors/ComponentEditor/common/SystemVerilogExpressionParser.h>

#include <IPXACTmodels/validators/ParameterValidator2014.h>
#include <IPXACTmodels/parameter.h>

class tst_ParameterValidator2014 : public QObject
{
    Q_OBJECT

public:
    tst_ParameterValidator2014();

private slots:

    void testValueIsValidForGivenType();
    void testValueIsValidForGivenType_data();

private:

    bool errorIsNotFoundInErrorlist(QString const& expectedError, QStringList const& errorlist);

};

//-----------------------------------------------------------------------------
// Function: tst_ParameterValidator2014::tst_ParameterValidator2014()
//-----------------------------------------------------------------------------
tst_ParameterValidator2014::tst_ParameterValidator2014()
{

}

//-----------------------------------------------------------------------------
// Function: tst_ParameterValidator2014::testValueIsValidForGivenType()
//-----------------------------------------------------------------------------
void tst_ParameterValidator2014::testValueIsValidForGivenType()
{
    QFETCH(QString, value);
    QFETCH(QString, type);    
    QFETCH(bool, isValid);

    QSharedPointer<ExpressionParser> parser(new SystemVerilogExpressionParser());

    ParameterValidator2014 validator(parser);
    QCOMPARE(validator.hasValidValueForType(value, type), isValid);

    if (!isValid && !value.isEmpty())
    {
        Parameter* parameter = new Parameter();
        parameter->setName("param");
        parameter->setType(type);
        parameter->setValue(value);

        QStringList errorlist = validator.findErrorsIn(parameter, "test", 
            QSharedPointer<QList<QSharedPointer<Choice> > >());

        QString expectedError = "Value " + value + " is not valid for type " + type + " in parameter param within test";
        if (errorIsNotFoundInErrorlist(expectedError, errorlist))
        {
            QFAIL("No error message found.");
        }
        delete parameter;
    }
}

//-----------------------------------------------------------------------------
// Function: tst_ParameterValidator2014::testValueIsValidForGivenType_data()
//-----------------------------------------------------------------------------
void tst_ParameterValidator2014::testValueIsValidForGivenType_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("type");
    QTest::addColumn<bool>("isValid");

    QTest::newRow("0 is valid for bit type") << "0" << "bit" << true;
    QTest::newRow("1 is valid for bit type") << "1" << "bit" << true;
    QTest::newRow("'1 is valid for bit type") << "'1" << "bit" << true;
    QTest::newRow("Binary 'b0 is valid for bit type") << "'b0" << "bit" << true;
    QTest::newRow("Binary 'b1 is valid for bit type") << "'b1" << "bit" << true;
    QTest::newRow("Binary 2'b11 with size is valid for bit type") << "2'b11" << "bit" << true;
    
    QTest::newRow("Binary 'b11 without size is not valid for bit type") << "'b11" << "bit" << false;
    QTest::newRow("Binary 3'b13 is not valid for bit type") << "3'b13" << "bit" << false;
    QTest::newRow("Binary with hexadecimal numbers is not valid for bit type") << "3'b3f" << "bit" << false;

    QTest::newRow("Hexadecimal 'h1 is valid for bit type") << "'h1" << "bit" << true;
    QTest::newRow("Hexadecimal with size is valid for bit type") << "8'h11" << "bit" << true;
    QTest::newRow("Hexadecimal with size and numbers 1-9 is valid for bit type") << "16'h1289" << "bit" << true;
    QTest::newRow("Hexadecimal with size and a-f characters is valid for bit type") << "8'haf" << "bit" << true;
    QTest::newRow("Hexadecimal with size and upper case is valid for bit type") << "8'hED" << "bit" << true;

    QTest::newRow("Hexadecimal without size is invalid for bit type") << "'h11" << "bit" << false;
    QTest::newRow("2 is invalid for bit type") << "2" << "bit" << false;
    QTest::newRow("-1 is invalid for bit type") << "-1" << "bit" << false;
    QTest::newRow("text is invalid for bit type") << "some text" << "bit" << false;
    QTest::newRow("string is invalid for bit type") << "\"some other text\"" << "bit" << false;
    QTest::newRow("Expression is valid for bit type") << "'h1 + 'h1 - 'b1" << "bit" << true;

    QTest::newRow("0 is valid for byte type") << "0" << "byte" << true;
    QTest::newRow("1 is valid for byte type") << "1" << "byte" << true;
    QTest::newRow("64 is valid for byte type") << "64" << "byte" << true;
    QTest::newRow("127 is valid for byte type") << "127" << "byte" << true;
    QTest::newRow("128 is invalid for byte type") << "128" << "byte" << false;
    QTest::newRow("-1 is valid for byte type") << "-1" << "byte" << true;
    QTest::newRow("-100 is valid for byte type") << "-100" << "byte" << true;
    QTest::newRow("-128 is valid for byte type") << "-128" << "byte" << true;
    QTest::newRow("-129 is invalid for byte type") << "-129" << "byte" << false;
    QTest::newRow("-9999999999 is invalid for byte type") << "-9999999999" << "byte" << false;
    QTest::newRow("Expression is valid for byte type") << "'h01 + 'h0E" << "byte" << true;

    QTest::newRow("0 is valid for shortint type") << "0" << "shortint" << true;
    QTest::newRow("1 is valid for shortint type") << "1" << "shortint" << true;
    QTest::newRow("128 is valid for shortint type") << "128" << "shortint" << true;
    QTest::newRow("32767 is valid for shortint type") << "32767" << "shortint" << true;
    QTest::newRow("32768 is invalid for shortint type") << "32768" << "shortint" << false;
    QTest::newRow("-1 is valid for shortint type") << "-1" << "shortint" << true;
    QTest::newRow("-129 is valid for shortint type") << "-129" << "shortint" << true;
    QTest::newRow("-32768 is valid for shortint type") << "-32768" << "shortint" << true;
    QTest::newRow("-32769 is valid for shortint type") << "-32769" << "shortint" << false;
    QTest::newRow("Expression is valid for shortint type") << "12 + 12" << "shortint" << true;

    QTest::newRow("0 is valid for int type") << "0" << "int" << true;
    QTest::newRow("1 is valid for int type") << "1" << "int" << true;
    QTest::newRow("32768 is valid for int type") << "32768" << "int" << true;
    QTest::newRow("2147483647 is valid for int type") << "2147483647" << "int" << true;
    QTest::newRow("2147483648 is invalid for int type") << "2147483648" << "int" << false;
    QTest::newRow("-1 is valid for int type") << "-1" << "int" << true;
    QTest::newRow("-32769 is valid for int type") << "-32769" << "int" << true;
    QTest::newRow("-2147483648 is valid for int type") << "-2147483648" << "int" << true;
    QTest::newRow("-2147483649 is invalid for int type") << "-2147483649" << "int" << false;
    QTest::newRow("Expression is valid for int type") << "12 + 12" << "int" << true;

    QTest::newRow("0 is valid for longint type") << "0" << "longint" << true;
    QTest::newRow("1 is valid for longint type") << "1" << "longint" << true;
    QTest::newRow("2147483648 is valid for longint type") << "2147483648" << "longint" << true;
    //QTest::newRow("9223372036854775807 is valid for longint type") << "9223372036854775807" << "longint" << true;
    //QTest::newRow("9223372036854775808 is invalid for longint type") << "9223372036854775808" << "longint" << false;
    QTest::newRow("-1 is valid for longint type") << "-1" << "longint" << true;
    QTest::newRow("-2147483649 is valid for longint type") << "-2147483649" << "longint" << true;
    QTest::newRow("-9223372036854775808 is valid for longint type") << "-9223372036854775808" << "longint" << true;
    //QTest::newRow("-9223372036854775809 is invalid for longint type") << "-9223372036854775809" << "longint" << false;
    QTest::newRow("Expression is valid for longint type") << "12 + 12" << "longint" << true;

    QTest::newRow("Emtpy is invalid for string type") << "" << "string" << false;
    QTest::newRow("String without double quotes is invalid") << "text" << "string" << false;
    QTest::newRow("Emtpy string is valid for string type") << "\"\"" << "string" << true;
    QTest::newRow("String in double quotes is valid") << "\"text\"" << "string" << true;
    QTest::newRow("Decimal number is invalid for string type") << "1" << "string" << false;
    QTest::newRow("Expression is invalid for string type") << "12 + 12" << "string" << false;
}

//-----------------------------------------------------------------------------
// Function: tst_ParameterValidator2014::errorIsNotFoundInErrorlist()
//-----------------------------------------------------------------------------
bool tst_ParameterValidator2014::errorIsNotFoundInErrorlist(QString const& expectedError, QStringList const& errorlist)
{
    if (!errorlist.contains(expectedError))
    {
        qDebug() << "The following error:" << endl << expectedError << endl << "was not found in errorlist:";
        foreach(QString error, errorlist)
        {
            qDebug() << error; 
        }
        return true;
    }

    return false;
}

QTEST_APPLESS_MAIN(tst_ParameterValidator2014)

#include "tst_ParameterValidator2014.moc"