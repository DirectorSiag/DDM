#include "obmHandler.h"
#include "qdebug.h"
#include <cmath>

OBMHandler::OBMHandler() {
    obmPosition = {0.0f, 0.0f};
    range = 4;
}

void OBMHandler::updatePosition(QPair<float, float> newPosition)
{
    float valorX = newPosition.first / 64.0f;
    float valorY = newPosition.second / 64.0f;

    // Mantener el signo en ambos ejes
    float deltaPositionX = valorX * std::abs(valorX) * range;
    float deltaPositionY = valorY * std::abs(valorY) * range;

    obmPosition.xPosition += deltaPositionX;
    obmPosition.yPosition += deltaPositionY;

}

void OBMHandler::setPosition(QPair<float,float> newPosition){
    obmPosition.xPosition = newPosition.first;
    obmPosition.yPosition = newPosition.second;
}

QPair<float, float> OBMHandler::getPosition()
{
    return { obmPosition.xPosition, obmPosition.yPosition };
}

void OBMHandler::updateRange(int newRange)
{
    if(newRange != range){
        range = newRange;
        qDebug() << "new Range for the boys" << range;
    }
}

Track* OBMHandler::OBMAssociationProcess(CommandContext* ctx) {
    if (!ctx) return nullptr;

    auto& tracks = ctx->getTracks();
    for (Track& tr : tracks) {
        const double d = getDistanceFromTrack(tr);
        if (d < 0.02) {
            return &tr;
        }
    }
    return nullptr;
}


double OBMHandler::getDistanceFromTrack(const Track& t) const {
    const double dx = static_cast<double>(obmPosition.xPosition) - t.getX();
    const double dy = static_cast<double>(obmPosition.yPosition) - t.getY();
    const double d  = std::hypot(dx, dy);
    return d / static_cast<double>(range);
}
