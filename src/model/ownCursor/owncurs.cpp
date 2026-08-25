#include "owncurs.h"

OwnCurs::OwnCurs(CommandContext* context, OBMHandler* newObm, QObject* parent)
    : QObject(parent), ctx(context), obm(newObm) {}

void OwnCurs::cuOrOffCent() {
    qDebug() << "OwnCurs << cuOrOffCent PRESSED";
    if (!cursorRef || !obm) return;
    const QPair<float, float> pos = obm->getPosition();
    cursorRef->get().setCoordinates(QPair<qfloat16, qfloat16>(
        static_cast<qfloat16>(pos.first), static_cast<qfloat16>(pos.second)));
}

void OwnCurs::cuOrCent(){
    if (!cursorRef) return;
    cursorRef->get().setCoordinates(QPair<qfloat16, qfloat16>(qfloat16(0.0f), qfloat16(0.0f)));
}

void OwnCurs::ownCursActive(bool value)
{
    qDebug() << "OWNCURS::ACTIVE";
    if (!cursorRef) {
        // Opción A: emplace (sin objeto temporal)
        CursorEntity& ref = ctx->emplaceCursorFront(
            QPair<qfloat16,qfloat16>(qfloat16(0), qfloat16(0)), // coordenadas
            qfloat16(45.0f),   // ángulo
            qfloat16(30.0f),   // largo
            0,                 // tipo de línea
            1,                  // id
            false);
        cursorRef = std::ref(ref);

        // (Opcional) Si querés asegurarte de una pos exacta:
        // cursorRef->get().setCoordinates(ORIGIN);
    }
    // Si ya existe, no hacemos nada: ya tenemos la referencia al guardado en ctx
    cursorRef->get().setActive(value);
}

void OwnCurs::updateHandwheel(const QPair<qfloat16, qfloat16>& update){
    if (!cursorRef) return;
    cursorRef->get().setCursorAngle(update.first);
    cursorRef->get().setCursorLength(update.second);
}
