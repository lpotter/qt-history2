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
****************************************************************************/

#include <cmath>

#include <qnumeric.h>

#include "AbstractFloat.h"
#include "AnyURI.h"
#include "Base64Binary.h"
#include "Boolean.h"
#include "CommonValues.h"
#include "Date.h"
#include "DateTime.h"
#include "DayTimeDuration.h"
#include "Decimal.h"
#include "Duration.h"
#include "GDay.h"
#include "GMonth.h"
#include "GMonthDay.h"
#include "GYear.h"
#include "GYearMonth.h"
#include "HexBinary.h"
#include "Integer.h"
#include "AtomicString.h"
#include "SchemaTime.h"
#include "UntypedAtomic.h"
#include "YearMonthDuration.h"

#include "AtomicCasters.h"

using namespace Patternist;

Item ToUntypedAtomicCaster::castFrom(const Item &from,
                                          const PlainSharedPtr<DynamicContext> &) const
{
    return UntypedAtomic::fromValue(from.stringValue());
}

Item ToAnyURICaster::castFrom(const Item &from,
                                   const PlainSharedPtr<DynamicContext> &) const
{
    return AnyURI::fromLexical(from.stringValue());
}

Item Base64BinaryToHexBinaryCaster::castFrom(const Item &from,
                                                  const PlainSharedPtr<DynamicContext> &) const
{
    return HexBinary::fromValue(from.as<Base64Binary>()->asByteArray());
}

Item StringToHexBinaryCaster::castFrom(const Item &from,
                                            const PlainSharedPtr<DynamicContext> &context) const
{
    return HexBinary::fromLexical(context->namePool(), from.stringValue());
}

Item HexBinaryToBase64BinaryCaster::castFrom(const Item &from,
                                                  const PlainSharedPtr<DynamicContext> &) const
{
    return Base64Binary::fromValue(from.as<Base64Binary>()->asByteArray());
}

Item StringToBase64BinaryCaster::castFrom(const Item &from,
                                               const PlainSharedPtr<DynamicContext> &) const
{
    return Base64Binary::fromLexical(from.stringValue());
}

Item NumericToBooleanCaster::castFrom(const Item &from,
                                           const PlainSharedPtr<DynamicContext> &) const
{
    const xsDouble val = from.as<Numeric>()->toDouble();
    if(Double::isEqual(val, 0.0) || qIsNaN(val))
        return CommonValues::BooleanFalse;
    else
        return CommonValues::BooleanTrue;
}

Item StringToBooleanCaster::castFrom(const Item &from,
                                          const PlainSharedPtr<DynamicContext> &) const
{
    return Boolean::fromLexical(from.stringValue());
}

Item StringToDecimalCaster::castFrom(const Item &from,
                                          const PlainSharedPtr<DynamicContext> &) const
{
    return Decimal::fromLexical(from.stringValue());
}

Item StringToIntegerCaster::castFrom(const Item &from,
                                          const PlainSharedPtr<DynamicContext> &) const
{
    return Integer::fromLexical(from.stringValue());
}

Item BooleanToDecimalCaster::castFrom(const Item &from,
                                           const PlainSharedPtr<DynamicContext> &context) const
{
    if(from.as<AtomicValue>()->evaluateEBV(context))
        return CommonValues::DecimalOne;
    else
        return CommonValues::DecimalZero;
}

Item BooleanToIntegerCaster::castFrom(const Item &from,
                                           const PlainSharedPtr<DynamicContext> &context) const
{
    if(from.as<AtomicValue>()->evaluateEBV(context))
        return CommonValues::IntegerOne;
    else
        return CommonValues::IntegerZero;
}

Item SelfToSelfCaster::castFrom(const Item &from,
                                     const PlainSharedPtr<DynamicContext> &) const
{
    return from;
}

Item StringToGYearCaster::castFrom(const Item &from,
                                        const PlainSharedPtr<DynamicContext> &) const
{
    return GYear::fromLexical(from.stringValue());
}

Item StringToGDayCaster::castFrom(const Item &from,
                                       const PlainSharedPtr<DynamicContext> &) const
{
    return GDay::fromLexical(from.stringValue());
}

Item StringToGMonthCaster::castFrom(const Item &from,
                                         const PlainSharedPtr<DynamicContext> &) const
{
    return GMonth::fromLexical(from.stringValue());
}

Item StringToGYearMonthCaster::castFrom(const Item &from,
                                             const PlainSharedPtr<DynamicContext> &) const
{
    return GYearMonth::fromLexical(from.stringValue());
}

Item StringToGMonthDayCaster::castFrom(const Item &from,
                                            const PlainSharedPtr<DynamicContext> &) const
{
    return GMonthDay::fromLexical(from.stringValue());
}

Item StringToDateTimeCaster::castFrom(const Item &from,
                                           const PlainSharedPtr<DynamicContext> &) const
{
    return DateTime::fromLexical(from.stringValue());
}

Item StringToTimeCaster::castFrom(const Item &from,
                                       const PlainSharedPtr<DynamicContext> &) const
{
    return SchemaTime::fromLexical(from.stringValue());
}

Item StringToDateCaster::castFrom(const Item &from,
                                       const PlainSharedPtr<DynamicContext> &) const
{
    return Date::fromLexical(from.stringValue());
}

Item StringToDurationCaster::castFrom(const Item &from,
                                           const PlainSharedPtr<DynamicContext> &) const
{
    return Duration::fromLexical(from.stringValue());
}

Item StringToDayTimeDurationCaster::castFrom(const Item &from,
                                                  const PlainSharedPtr<DynamicContext> &) const
{
    return toItem(DayTimeDuration::fromLexical(from.stringValue()));
}

Item AbstractDurationToDayTimeDurationCaster::castFrom(const Item &from,
                                                       const PlainSharedPtr<DynamicContext> &) const
{
    const AbstractDuration *const val = from.as<AbstractDuration>();

    return toItem(DayTimeDuration::fromComponents(val->isPositive(),
                                                  val->days(),
                                                  val->hours(),
                                                  val->minutes(),
                                                  val->seconds(),
                                                  val->mseconds()));
}

Item AbstractDurationToYearMonthDurationCaster::castFrom(const Item &from,
                                                              const PlainSharedPtr<DynamicContext> &) const
{
    const AbstractDuration *const val = from.as<AbstractDuration>();

    return toItem(YearMonthDuration::fromComponents(val->isPositive(),
                                                    val->years(),
                                                    val->months()));
}

Item AbstractDurationToDurationCaster::castFrom(const Item &from,
                                                     const PlainSharedPtr<DynamicContext> &) const
{
    const AbstractDuration *const val = from.as<AbstractDuration>();

    return Duration::fromComponents(val->isPositive(),
                                    val->years(),
                                    val->months(),
                                    val->days(),
                                    val->hours(),
                                    val->minutes(),
                                    val->seconds(),
                                    val->mseconds());
}

Item StringToYearMonthDurationCaster::castFrom(const Item &from,
                                                    const PlainSharedPtr<DynamicContext> &) const
{
    return YearMonthDuration::fromLexical(from.stringValue());
}

Item AbstractDateTimeToGYearCaster::castFrom(const Item &from,
                                                  const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return GYear::fromDateTime(dt);
}

Item AbstractDateTimeToGYearMonthCaster::castFrom(const Item &from,
                                                       const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return GYearMonth::fromDateTime(dt);
}

Item AbstractDateTimeToGMonthCaster::castFrom(const Item &from,
                                                   const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return GMonth::fromDateTime(dt);
}

Item AbstractDateTimeToGMonthDayCaster::castFrom(const Item &from,
                                                      const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return GMonthDay::fromDateTime(dt);
}

Item AbstractDateTimeToGDayCaster::castFrom(const Item &from,
                                                 const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return GDay::fromDateTime(dt);
}

Item AbstractDateTimeToDateTimeCaster::castFrom(const Item &from,
                                                     const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(false);

    return DateTime::fromDateTime(dt);
}

Item AbstractDateTimeToDateCaster::castFrom(const Item &from,
                                                 const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(true);

    return Date::fromDateTime(dt);
}

Item AbstractDateTimeToTimeCaster::castFrom(const Item &from,
                                                 const PlainSharedPtr<DynamicContext> &) const
{
    QDateTime dt(from.as<AbstractDateTime>()->toDateTime());
    // TODO DT dt.setDateOnly(false);

    return SchemaTime::fromDateTime(dt);
}

// vim: et:ts=4:sw=4:sts=4
