#include "cursorservice.h"

#include "commandContext.h"
#include "model/utils/RadarMath.h"

#include <QPair>
#include <QStringView>
#include <qfloat16.h>
#include <QJsonArray>
#include <QJsonObject>

CursorService::CursorService(CommandContext* context)
    : m_context(context)
{
    Q_ASSERT(m_context);
}

QJsonArray CursorService::serializeCursors() const
{
    QJsonArray out;
    const auto& cursors = m_context->getCursors();
    for (const CursorEntity& c : cursors) {
        QJsonObject obj;
        obj["id"] = c.getCursorId();
        obj["type"] = c.getLineType();
        obj["angle"] = double(c.getCursorAngle());
        obj["length"] = double(c.getCursorLength());
        obj["x"] = double(c.getCoordinates().first);
        obj["y"] = double(c.getCoordinates().second);
        obj["active"] = c.isActive();
        out.append(obj);
    }
    return out;
}

CursorOperationResult CursorService::createCursor(const CursorCreateRequest& request)
{
    if (request.type < 0 || request.type > 7) {
        return {false, "INVALID_TYPE", "El tipo de cursor debe estar entre 0 y 7", -1, QString()};
    }

    if (request.length < 0.0 || request.length > 256.0) {
        return {false, "INVALID_LENGTH", "El largo de cursor debe estar entre 0.0 y 256.0", -1, QString()};
    }

    const double normalizedAzimut = RadarMath::normalizeAngle360(request.azimut);
    const int cursorId = m_context->nextCursorId++;

    try {
        m_context->emplaceCursorFront(
            QPair<qfloat16, qfloat16>(qfloat16(request.x), qfloat16(request.y)),
            qfloat16(normalizedAzimut),
            qfloat16(request.length),
            request.type,
            cursorId,
            true
        );
    } catch (const std::exception& ex) {
        return {false, "BACKEND_ERROR", QString("Error interno al crear cursor: %1").arg(ex.what()), -1, QString()};
    }

    CursorOperationResult result;
    result.success = true;
    result.cursorId = cursorId;
    result.lineId = lineIdFromCursorId(cursorId);
    result.x = request.x;
    result.y = request.y;
    result.angle = normalizedAzimut;
    result.length = request.length;
    result.type = request.type;
    result.active = true;
    return result;
}

CursorOperationResult CursorService::deleteCursorById(int cursorId)
{
    if (cursorId < 0) {
        return {false, "INVALID_CURSOR_ID", "El id de cursor debe ser no negativo", -1, QString()};
    }

    if (!m_context->eraseCursorById(cursorId)) {
        return {
            false,
            "CURSOR_NOT_FOUND",
            QString("Cursor no encontrado: %1").arg(cursorId),
            cursorId,
            lineIdFromCursorId(cursorId)
        };
    }

    CursorOperationResult result;
    result.success = true;
    result.cursorId = cursorId;
    result.lineId = lineIdFromCursorId(cursorId);
    return result;
}

QString CursorService::lineIdFromCursorId(int cursorId)
{
    return QString("LINE_%1").arg(cursorId);
}

int CursorService::cursorIdFromLineId(const QString& lineId, bool* ok)
{
    return QStringView(lineId).mid(5).toInt(ok);
}
