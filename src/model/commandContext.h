#pragma once
#include <QTextStream>
#include <QStringConverter>
#include <QString>
#include <QPointF>
#include <QPair>
#include <deque>
#include <utility>
#include <unordered_map>

#include "entities/track.h"
#include "entities/cursorEntity.h"

struct CommandContext {
    CommandContext() : out(stdout), err(stderr) {
        out.setEncoding(QStringConverter::Utf8);
        err.setEncoding(QStringConverter::Utf8);
    }

    // --- SITREP extra ---
    struct SitrepExtra { QString info; };
    std::unordered_map<int, SitrepExtra> sitrepExtra;

    inline QString sitrepInfo(int trackId) const {
        auto it = sitrepExtra.find(trackId);
        return (it != sitrepExtra.end()) ? it->second.info : QString("-");
    }

    inline void setSitrepInfo(int trackId, const QString& text) {
        sitrepExtra[trackId].info = text;
    }

    QTextStream out;
    QTextStream err;
    QString     lastCommandLine;
    quint64     lastCommandHash = 0;
    int         commandCounter  = 1;

    std::deque<CursorEntity> cursors;
    std::deque<Track> tracks;
    int               nextTrackId = 1;

    int               nextCursorId = 2;

    double centerX = 0.0;
    double centerY = 0.0;

    inline std::deque<Track>& getTracks() { return tracks; }
    inline const std::deque<Track>& getTracks() const { return tracks; }

    inline std::deque<CursorEntity>& getCursors() { return cursors; }
    inline const std::deque<CursorEntity>& getCursors() const { return cursors; }

    inline CursorEntity& addCursorFront(const CursorEntity& c) {
        qDebug() << "agregando cursor";
        cursors.push_front(c);
        qDebug() << "termine de agregar";
        return cursors.front();
    }

    template <typename... Args>
    inline CursorEntity& emplaceCursorFront(Args&&... args) {
        qDebug() << "agregando cursor (emplace)";
        cursors.emplace_front(std::forward<Args>(args)...);
        qDebug() << "termine de agregar (emplace)";
        qDebug() << "cursors size =" << cursors.size();
        return cursors.front();
    }

    inline Track& addTrackFront(const Track& t) {
        tracks.push_front(t);
        return tracks.front();
    }

    template <typename... Args>
    inline Track& emplaceTrackFront(Args&&... args) {
        tracks.emplace_front(std::forward<Args>(args)...);
        return tracks.front();
    }

    inline QPointF center() const { return QPointF(centerX, centerY); }

    inline void setCenter(QPair<float,float> c){
        centerX = c.first;
        centerY = c.second;
    }

    inline void resetCenter(){ setCenter({0.0f, 0.0f}); }

    inline Track* findTrackById(int id) {
        for (Track& t : tracks) if (t.getId() == id) return &t;
        return nullptr;
    }
    inline const Track* findTrackById(int id) const {
        for (const Track& t : tracks) if (t.getId() == id) return &t;
        return nullptr;
    }

    inline bool eraseTrackById(int id) {
        for (auto it = tracks.begin(); it != tracks.end(); ++it) {
            if (it->getId() == id) { tracks.erase(it); return true; }
        }
        return false;
    }

    inline bool eraseCursorById(int id) {
        for (auto it = cursors.begin(); it != cursors.end(); ++it) {
            if (it->getCursorId() == id) { cursors.erase(it); return true; }
        }
        return false;
    }

    inline Track* getNextTrackById(int currentId) {
        if (tracks.empty()) return nullptr;
        std::size_t i = 0;
        for (; i < tracks.size(); ++i) {
            if (tracks[i].getId() == currentId) break;
        }
        if (i == tracks.size()) return nullptr;
        const std::size_t j = (i + 1) % tracks.size();
        return &tracks[j];
    }

    inline void updateTracks(double deltaTime){
        for(Track& track : tracks){
            track.updatePosition(deltaTime);
        }
    }
};


