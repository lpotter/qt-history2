/****************************************************************************
**
** Copyright (C) 2006-$THISYEAR$ $TROLLTECH$. All rights reserved.
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
***************************************************************************
*/

#ifndef Patternist_AbstractDateTime_H
#define Patternist_AbstractDateTime_H

#include <QDateTime>
#include <QRegExp>

#include "Item.h"

QT_BEGIN_HEADER 

namespace Patternist
{
    /**
     * @short Base class for classes implementing values related to time, date or both.
     *
     * @see <a href="http://www.w3.org/TR/xmlschema-2/#dateTime">XML Schema
     * Part 2: Datatypes Second Edition, 3.2.7 dateTime</a>
     * @see <a href="http://www.w3.org/TR/xpath-datamodel/#dates-and-times">XQuery
     * 1.0 and XPath 2.0 Data Model (XDM), 3.3.2 Dates and Times</a>
     * @see <a href="http://www.cl.cam.ac.uk/~mgk25/iso-time.html">A summary of
     * the international standard date and time notation, Markus Kuhn</a>
     * @see <a href="http://en.wikipedia.org/wiki/Iso_date">ISO 8601,
     * From Wikipedia, the free encyclopedia</a>
     * @author Frans Englich <fenglich@trolltech.com>
     * @ingroup Patternist_xdm
     */
    class AbstractDateTime : public AtomicValue
    {
    public:
        typedef PlainSharedPtr<AbstractDateTime> Ptr;

        AbstractDateTime(const QDateTime &dateTime);

        enum
        {
            DefaultYear     = 2000,
            DefaultMonth    = 1,
            DefaultDay      = 1
        };

        /**
         * @returns the date time this class represents, as a QDateTime.
         */
        inline const QDateTime &toDateTime() const
        {
            return m_dateTime;
        }


        /**
         * @short Acts as a mapping table for AbstractDateTime::create()
         * and describes where certain fields in a QRegExp pattern can be found
         * for a particular W3C XML Schema date/time type.
         *
         * @author Frans Englich <fenglich@trolltech.com>
         * @ingroup Patternist_xdm
         */
        class CaptureTable
        {
        public:
            CaptureTable(const QRegExp &exp,
                         const qint8 zoneOffsetSignP,
                         const qint8 zoneOffsetHourP,
                         const qint8 zoneOffsetMinuteP,
                         const qint8 zoneOffsetUTCSymbolP,
                         const qint8 yearP,
                         const qint8 monthP = -1,
                         const qint8 dayP = -1,
                         const qint8 hourP = -1,
                         const qint8 minutesP = -1,
                         const qint8 secondsP = -1,
                         const qint8 msecondsP = -1) : regExp(exp),
                                                       zoneOffsetSign(zoneOffsetSignP),
                                                       zoneOffsetHour(zoneOffsetHourP),
                                                       zoneOffsetMinute(zoneOffsetMinuteP),
                                                       zoneOffsetUTCSymbol(zoneOffsetUTCSymbolP),
                                                       year(yearP),
                                                       month(monthP),
                                                       day(dayP),
                                                       hour(hourP),
                                                       minutes(minutesP),
                                                       seconds(secondsP),
                                                       mseconds(msecondsP)
            {
                Q_ASSERT(exp.isValid());
            }

            const QRegExp regExp;
            const qint8 zoneOffsetSign;
            const qint8 zoneOffsetHour;
            const qint8 zoneOffsetMinute;
            const qint8 zoneOffsetUTCSymbol;
            const qint8 year;
            const qint8 month;
            const qint8 day;
            const qint8 hour;
            const qint8 minutes;
            const qint8 seconds;
            const qint8 mseconds;

        private:
            Q_DISABLE_COPY(CaptureTable)
        };

        /**
         * @returns m_dateTime's time part converted to string. This is for
         * example "12" or "01.023".
         */
        QString timeToString() const;

        /**
         * @returns m_dateTime's date part converted to string. This is for
         * example "2004-05-12" or "-2004-05-12".
         */
        QString dateToString() const;

        /**
         * Serializes the milli seconds @p msecs into a string representation. For
         * example, if @p msecs is 1, ".001" is returned; if @p msecs is 100 then
         * is ".1" returned.
         */
        static QString serializeMSeconds(const MSecondProperty msecs);

        /**
         * A factory function for creating instances that are of the dynamic
         * type of this class, that represents @p dt.
         *
         * The default implementation performs an assert() call. This function
         * is not pure virtual because all sub-classes do not use it.
         */
        virtual Item fromValue(const QDateTime &dt) const;

        /**
         * Determines whether @p dt is a date-time that can be represented,
         * and isn't too early or too late. If it is valid, @c true is returned. Otherwise,
         * @c false is returned and @p message is set to contain a translated message for
         * human consumption, describing the error.
         */
        static bool isRangeValid(const QDate &date,
                                 QString &message);

    protected:

        /**
         * @returns m_dateTime' zone offset converted to string, as per the
         * the W3C XML Schema types. This is for
         * example "Z" or "+12.00"(depending on m_dateTime).
         */
        QString zoneOffsetToString() const;

        static QDateTime create(AtomicValue::Ptr &errorMessage,
                                const QString &lexicalSource,
                                const CaptureTable &captTable);

        /**
         * @short Makes the QDateTime::timeSpec() and QDateTime::zoneOffset()
         * of @p ot * consistent to @p from.
         */
        static void copyTimeSpec(const QDateTime &from,
                                 QDateTime &to);

        const QDateTime m_dateTime;

    private:
        enum ZoneOffsetParseResult
        {
            /**
             * syntax or logical error was encountered.
             */
            Error,
            /**
             * It's a valid offset from UTC.
             */
            Offset,

            /**
             * No zone offset was specified, it's an implementation defined zone offset.
             */
            LocalTime,
            UTC
        };

        /**
         * @short Parses the zone offset. All types use zone offsets.
         *
         * If result is set to Offset, the offset is returned, otherwise
         * the return value is undefined.
         *
         * The offset is in seconds.
         */
        static ZOTotal parseZoneOffset(ZoneOffsetParseResult &result,
                                       const QStringList &capts,
                                       const CaptureTable &captTable);

        static inline void setUtcOffset(QDateTime &result,
                                        const ZoneOffsetParseResult zoResult,
                                        const int zoOffset);
    };
}

QT_END_HEADER 

#endif
// vim: et:ts=4:sw=4:sts=4
