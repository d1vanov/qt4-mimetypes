/**************************************************************************
**
** This file is part of QMime
**
** Based on Qt Creator source code
**
** Qt Creator Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
**************************************************************************/

#ifndef QMIMEDATABASE_P_H
#define QMIMEDATABASE_P_H

#include <QtCore/QMultiHash>
#include <QtCore/QMutex>

#include "qmimetype.h"
#include "qmimetype_p.h"
#include "qmimeglobpattern_p.h"

QT_BEGIN_NAMESPACE

#define MIN_MATCH_WEIGHT 50

// MimeMapEntry: Entry of a type map, consisting of type.
struct MimeMapEntry
{
    inline MimeMapEntry(const QMimeType &aType = QMimeType()) :
        type(aType)
    {}

    QMimeType type;
};


class QMimeDatabasePrivate
{
    Q_DISABLE_COPY(QMimeDatabasePrivate)
    friend class QMimeDatabase;
    friend class QMimeDatabaseBuilder;

public:
    QMimeDatabasePrivate();
    ~QMimeDatabasePrivate();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);
    bool addMimeType(const QMimeType &mt);

    QStringList filterStrings() const;

    QList<QMimeType> mimeTypes() const;

private:
    typedef QHash<QString, QString> AliasMap;
    typedef QMultiHash<QString, QString> ParentChildrenMap;

    bool addMimeTypes(QIODevice *device, const QString &fileName, QString *errorMessage);
    inline QString resolveAlias(const QString &name) const
    { return aliasMap.value(name, name); }

    QMimeType findByType(const QString &type) const;
    QMimeType findByFile(const QFileInfo &f, unsigned *priorityPtr) const;
    QMimeType findByData(const QByteArray &data, unsigned *priorityPtr) const;
    QStringList findByName(const QString &fileName) const;
    void findFromOtherPatternList(QStringList &matchingMimeTypes,
                                  const QString &fileName,
                                  QString &foundExt,
                                  bool highWeight) const;

    QMimeAllGlobPatterns m_mimeTypeGlobs;

    QHash<QString, MimeMapEntry*> typeMimeTypeMap;
    AliasMap aliasMap;
    ParentChildrenMap parentChildrenMap;
    QMutex mutex;
};


class QMimeDatabaseBuilder
{
    Q_DISABLE_COPY(QMimeDatabaseBuilder)
public:
    QMimeDatabaseBuilder();
    ~QMimeDatabaseBuilder();

    bool addMimeTypes(const QString &fileName, QString *errorMessage);
    bool addMimeTypes(QIODevice *device, QString *errorMessage);

    void addGlobPattern(const QMimeGlobPattern& glob);

private:
    QMimeDatabasePrivate *const d;
};

QT_END_NAMESPACE

#endif // QMIMEDATABASE_P_H
