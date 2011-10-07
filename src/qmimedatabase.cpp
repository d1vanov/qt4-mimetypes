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

#include "qmimedatabase.h"
#include "qmimedatabase_p.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSet>
#include <QtCore/QDebug>

#include <algorithm>
#include <functional>

#include "magicmatcher_p.h"
#include "mimetypeparser_p.h"
#include "qmimetype_p.h"

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QMimeDatabasePrivate, staticMimeDataBase)

QMimeDatabasePrivate::QMimeDatabasePrivate() :
    maxLevel(-1)
{
    // Assign here to avoid non-local static data initialization issues.
//    kModifiedMimeTypesPath = ICore::instance()->userResourcePath() + QLatin1String("/mimetypes/");
#warning TODO: FIX!!!
}

QMimeDatabasePrivate::~QMimeDatabasePrivate()
{
    qDeleteAll(typeMimeTypeMap);
}

bool QMimeDatabasePrivate::addMimeTypes(QIODevice *device, const QString &fileName, QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();

    MimeTypeParser parser(*this);
    return parser.parse(device, fileName, errorMessage);
}

bool QMimeDatabasePrivate::addMimeTypes(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(fileName, file.errorString());
        return false;
    }

    if (errorMessage)
        errorMessage->clear();

    return addMimeTypes(&file, fileName, errorMessage);
}

bool QMimeDatabasePrivate::addMimeTypes(QIODevice *device, QString *errorMessage)
{
    if (errorMessage)
        errorMessage->clear();

    return addMimeTypes(device, QLatin1String("<stream>"), errorMessage);
}

bool QMimeDatabasePrivate::addMimeType(const QMimeType &mt)
{
    if (!mt.isValid())
        return false;

    const QString &type = mt.type();

    // insert the type.
    typeMimeTypeMap.insert(type, new MimeMapEntry(mt));

    // Register the children, resolved via alias map. Note that it is still
    // possible that aliases end up in the map if the parent classes are not inserted
    // at this point (thus their aliases not known).
    foreach (const QString &subClassOf, mt.subClassOf())
        parentChildrenMap.insert(resolveAlias(subClassOf), type);

    // register aliasses
    foreach (const QString &alias, mt.aliases())
        aliasMap.insert(alias, type);

    maxLevel = -1; // Mark as dirty

    return true;
}

void QMimeDatabasePrivate::raiseLevelRecursion(MimeMapEntry &e, int level)
{
    if (e.level == MimeMapEntry::Dangling || e.level < level)
        e.level = level;

    if (maxLevel < level)
        maxLevel = level;

    // At all events recurse over children since nodes might have been added;
    // look them up in the type->MIME type map
    foreach (const QString &alias, parentChildrenMap.values(e.type.type())) {
        MimeMapEntry *entry = typeMimeTypeMap.value(resolveAlias(alias));
        if (!entry) {
            qWarning("%s: Inconsistent MIME hierarchy detected, child %s of %s cannot be found.",
                     Q_FUNC_INFO, alias.toLocal8Bit().constData(), e.type.type().toLocal8Bit().constData());
        } else {
            raiseLevelRecursion(*entry, level + 1);
        }
    }
}

void QMimeDatabasePrivate::determineLevels()
{
    // Loop over toplevels and recurse down their hierarchies.
    // Determine top levels by subtracting the children from the parent
    // set. Note that a toplevel at this point might have 'subclassesOf'
    // set to some class that is not in the DB, so, checking for an empty
    // 'subclassesOf' set is not sufficient to find the toplevels.
    // First, take the parent->child entries  whose parent exists and build
    // sets of parents/children
    QSet<QString> parentSet, childrenSet;
    ParentChildrenMap::const_iterator pit = parentChildrenMap.constBegin();
    for ( ; pit != parentChildrenMap.constEnd(); ++pit) {
        if (typeMimeTypeMap.contains(pit.key())) {
            parentSet.insert(pit.key());
            childrenSet.insert(pit.value());
        }
    }

    foreach (const QString &topLevel, parentSet.subtract(childrenSet)) {
        MimeMapEntry *entry = typeMimeTypeMap.value(resolveAlias(topLevel));
        if (!entry) {
            qWarning("%s: Inconsistent MIME hierarchy detected, top level element %s cannot be found.",
                     Q_FUNC_INFO, topLevel.toLocal8Bit().constData());
        } else {
            raiseLevelRecursion(*entry, 0);
        }
    }

    // move all danglings to top level
    foreach (MimeMapEntry *entry, typeMimeTypeMap) {
        if (entry->level == MimeMapEntry::Dangling)
            entry->level = 0;
    }
}

bool QMimeDatabasePrivate::setPreferredSuffix(const QString &typeOrAlias, const QString &suffix)
{
    TypeMimeTypeMap::iterator tit =  typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != typeMimeTypeMap.end()) {
        QMimeTypeData mimeTypeData = QMimeTypeData(tit.value()->type);
        mimeTypeData.preferredSuffix = suffix;
        tit.value()->type = QMimeType(mimeTypeData);
        return true;
    }
    return false;
}

// Returns a MIME type or Null one if none found
QMimeType QMimeDatabasePrivate::findByType(const QString &typeOrAlias) const
{
    const MimeMapEntry *entry = typeMimeTypeMap.value(resolveAlias(typeOrAlias));
    if (entry)
        return entry->type;
    return QMimeType();
}

// Helper for findByName
void QMimeDatabasePrivate::findFromOtherPatternList(QStringList &matchingMimeTypes,
                                                    const QString &fileName,
                                                    QString &foundExt,
                                                    bool highWeight) const
{
    const QMimeGlobPatternList &patternList = highWeight ? m_mimeTypeGlobs.m_highWeightGlobs : m_mimeTypeGlobs.m_lowWeightGlobs;

    int matchingPatternLength = 0;
    qint32 lastMatchedWeight = 0;
    if (!highWeight && !matchingMimeTypes.isEmpty()) {
        // We found matches in the fast pattern dict already:
        matchingPatternLength = foundExt.length() + 2; // *.foo -> length=5
        lastMatchedWeight = 50;
    }

    QMimeGlobPatternList::const_iterator it = patternList.constBegin();
    const QMimeGlobPatternList::const_iterator end = patternList.constEnd();
    for ( ; it != end; ++it ) {
        const QMimeGlobPattern &glob = *it;
        if (glob.matchFileName(fileName)) {
            const int weight = glob.weight();
            const QString pattern = glob.pattern();
            // Is this a lower-weight pattern than the last match? Stop here then.
            if (weight < lastMatchedWeight)
                break;
            if (lastMatchedWeight > 0 && weight > lastMatchedWeight) // can't happen
                qWarning() << "Assumption failed; globs2 weights not sorted correctly"
                           << weight << ">" << lastMatchedWeight;
            // Is this a shorter or a longer match than an existing one, or same length?
            if (pattern.length() < matchingPatternLength) {
                continue; // too short, ignore
            } else if (pattern.length() > matchingPatternLength) {
                // longer: clear any previous match (like *.bz2, when pattern is *.tar.bz2)
                matchingMimeTypes.clear();
                // remember the new "longer" length
                matchingPatternLength = pattern.length();
            }
            matchingMimeTypes.push_back(glob.mimeType());
            if (pattern.startsWith(QLatin1String("*.")))
                foundExt = pattern.mid(2);
        }
    }
}

QStringList QMimeDatabasePrivate::findByName(const QString &fileName) const
{
    // TODO parse globs file on demand here

    // First try the high weight matches (>50), if any.
    QStringList matchingMimeTypes;
    QString foundExt;
    findFromOtherPatternList(matchingMimeTypes, fileName, foundExt, true);
    if (matchingMimeTypes.isEmpty()) {

        // Now use the "fast patterns" dict, for simple *.foo patterns with weight 50
        // (which is most of them, so this optimization is definitely worth it)
        const int lastDot = fileName.lastIndexOf(QLatin1Char('.'));
        if (lastDot != -1) { // if no '.', skip the extension lookup
            const int ext_len = fileName.length() - lastDot - 1;
            const QString simpleExtension = fileName.right( ext_len ).toLower();
            // (toLower because fast matterns are always case-insensitive and saved as lowercase)

            matchingMimeTypes = m_mimeTypeGlobs.m_fastPatterns.value(simpleExtension);
            if (!matchingMimeTypes.isEmpty()) {
                foundExt = simpleExtension;
                // Can't return yet; *.tar.bz2 has to win over *.bz2, so we need the low-weight mimetypes anyway,
                // at least those with weight 50.
            }
        }

        // Finally, try the low weight matches (<=50)
        findFromOtherPatternList(matchingMimeTypes, fileName, foundExt, false);
    }
    //if (pMatchingExtension)
    //    *pMatchingExtension = foundExt;
    return matchingMimeTypes;
}

// Returns a MIME type or Null one if none found
QMimeType QMimeDatabasePrivate::findByData(const QByteArray &data, unsigned *priorityPtr) const
{
    // Is the hierarchy set up in case we find several matches?
    if (maxLevel < 0) {
        QMimeDatabasePrivate *db = const_cast<QMimeDatabasePrivate *>(this);
        db->determineLevels();
    }

    QMimeType candidate;

    for (int level = maxLevel; level >= 0; --level) {
        foreach (const MimeMapEntry *entry, typeMimeTypeMap) {
            if (entry->level == level) {
                const unsigned contentPriority = entry->type.d->matchesData(data);
                if (contentPriority && contentPriority > *priorityPtr) {
                    *priorityPtr = contentPriority;
                    candidate = entry->type;
                }
            }
        }
    }

    return candidate;
}

// Return all known suffixes
QStringList QMimeDatabasePrivate::suffixes() const
{
    QStringList rc;
    const TypeMimeTypeMap::const_iterator cend = typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = typeMimeTypeMap.constBegin(); it != cend; ++it)
        rc += it.value()->type.suffixes();
    return rc;
}

QList<QMimeGlobPattern> QMimeDatabasePrivate::globPatterns() const
{
    QList<QMimeGlobPattern> globPatterns;
    const TypeMimeTypeMap::const_iterator cend = typeMimeTypeMap.constEnd();
    for (TypeMimeTypeMap::const_iterator it = typeMimeTypeMap.constBegin(); it != cend; ++it)
        globPatterns.append(toGlobPatterns(it.value()->type.globPatterns(), it.value()->type.type()));
    return globPatterns;
}

void QMimeDatabasePrivate::setGlobPatterns(const QString &typeOrAlias,
                                           const QStringList &globPatterns)
{
    TypeMimeTypeMap::iterator tit =  typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != typeMimeTypeMap.end()) {
        QMimeTypeData mimeTypeData = QMimeTypeData(tit.value()->type);
        mimeTypeData.globPatterns = globPatterns;
        tit.value()->type = QMimeType(mimeTypeData);
    }
}

void QMimeDatabasePrivate::setMagicMatchers(const QString &typeOrAlias,
                                            const QList<QMimeMagicRuleMatcher> &matchers)
{
    TypeMimeTypeMap::iterator tit = typeMimeTypeMap.find(resolveAlias(typeOrAlias));
    if (tit != typeMimeTypeMap.end()) {
        QMimeTypeData mimeTypeData = QMimeTypeData(tit.value()->type);
        mimeTypeData.magicMatchers = matchers;
        tit.value()->type = QMimeType(mimeTypeData);
    }
}

// Returns a MIME type or Null one if none found
QMimeType QMimeDatabasePrivate::findByFile(const QFileInfo &f, unsigned *priorityPtr) const
{
    // First, glob patterns are evaluated. If there is a match with max weight,
    // this one is selected and we are done. Otherwise, the file contents are
    // evaluated and the match with the highest value (either a magic priority or
    // a glob pattern weight) is selected. Matching starts from max level (most
    // specific) in both cases, even when there is already a suffix matching candidate.
    *priorityPtr = 0;
    FileMatchContext context(f);

    // Pass 1) Try to match on suffix#type
    QStringList candidatesByName = findByName(f.fileName());

    // TODO REWRITE THIS METHOD, FOR PROPER GLOB-CONFLICT HANDLING

    QMimeType candidateByName;
    if (!candidatesByName.isEmpty()) {
        *priorityPtr = 50;
        candidateByName = findByType(candidatesByName.last());
    }

    // Pass 2) Match on content
    if (!f.isReadable())
        return candidateByName;

    if (candidateByName.matchesData(context.data()) > MIN_MATCH_WEIGHT)
        return candidateByName;

    unsigned priorityByName = *priorityPtr;
    QMimeType candidateByData(findByData(context.data(), priorityPtr));

    // ## BROKEN, PRIORITIES HAVE A DIFFERENT SCALE
    return priorityByName < *priorityPtr ? candidateByData : candidateByName;
}

QStringList QMimeDatabasePrivate::filterStrings() const
{
    QStringList rc;

    foreach (const MimeMapEntry *entry, typeMimeTypeMap) {
        const QString filterString = entry->type.filterString();
        if (!filterString.isEmpty())
            rc += filterString;
    }

    return rc;
}

QList<QMimeType> QMimeDatabasePrivate::mimeTypes() const
{
    QList<QMimeType> mimeTypes;

    foreach (const MimeMapEntry *entry, typeMimeTypeMap)
        mimeTypes.append(entry->type);

    return mimeTypes;
}

QList<QMimeGlobPattern> QMimeDatabasePrivate::toGlobPatterns(const QStringList &patterns, const QString &mimeType, int weight)
{
    QList<QMimeGlobPattern> globPatterns;
    foreach (const QString &pattern, patterns) {
        globPatterns.append(QMimeGlobPattern(pattern, mimeType, weight, Qt::CaseSensitive));
    }
    return globPatterns;
}

QStringList QMimeDatabasePrivate::fromGlobPatterns(const QList<QMimeGlobPattern> &globPatterns)
{
    QStringList patterns;
    foreach (const QMimeGlobPattern &globPattern, globPatterns)
        patterns.append(globPattern.pattern());
    return patterns;
}


/*!
    \class QMimeDatabase
    \brief MIME database to which the plugins can add the MIME types they handle.

    The class is protected by a QMutex and can therefore be accessed by threads.

    A good testcase is to run it over \c '/usr/share/mime/<*>/<*>.xml' on Linux.

    When adding a "text/plain" to it, the mimetype will receive a magic matcher
    that checks for text files that do not match the globs by heuristics.

    \section1 Design Considerations

    Storage requirements:
    \list
    \o Must be robust in case of incomplete hierarchies, dangling entries
    \o Plugins will not load and register their MIME types in order of inheritance.
    \o Multiple inheritance (several subClassOf) can occur
    \o Provide quick lookup by name
    \o Provide quick lookup by file type.
    \endlist

    This basically rules out some pointer-based tree, so the structure chosen is:
    \list
    \o An alias map QString->QString for mapping aliases to types
    \o A Map QString->MimeMapEntry for the types (MimeMapEntry being a pair of
       QMimeType and (hierarchy) level.
    \o A map  QString->QString representing parent->child relations (enabling
       recursing over children)
    \o Using strings avoids dangling pointers.
    \endlist

    The hierarchy level is used for mapping by file types. When findByFile()
    is first called after addMimeType() it recurses over the hierarchy and sets
    the hierarchy level of the entries accordingly (0 toplevel, 1 first
    order...). It then does several passes over the type map, checking the
    globs for maxLevel, maxLevel-1....until it finds a match (idea being to
    to check the most specific types first). Starting a recursion from the
    leaves is not suitable since it will hit parent nodes several times.

    \sa QMimeType, QMimeMagicRuleMatcher, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa BaseMimeTypeParser, MimeTypeParser
*/

QMimeDatabase::QMimeDatabase() :
    d(staticMimeDataBase())
{
}

QMimeDatabase::QMimeDatabase(QMimeDatabasePrivate *const theD) :
    d(theD)
{
}

QMimeDatabase::~QMimeDatabase()
{
    if (d != staticMimeDataBase()) {
        delete d;
    }

    d = 0;
}

QMimeDatabaseBuilder::QMimeDatabaseBuilder(QMimeDatabase *mimeDatabase) :
    d(mimeDatabase->data_ptr())
{
}

QMimeDatabaseBuilder::~QMimeDatabaseBuilder()
{
}

bool QMimeDatabaseBuilder::addMimeType(const QMimeType &mt)
{
    QMutexLocker locker(&d->mutex);

    return d->addMimeType(mt);
}

bool QMimeDatabaseBuilder::addMimeTypes(const QString &fileName, QString *errorMessage)
{
    QMutexLocker locker(&d->mutex);

    return d->addMimeTypes(fileName, errorMessage);
}

bool QMimeDatabaseBuilder::addMimeTypes(QIODevice *device, QString *errorMessage)
{
    QMutexLocker locker(&d->mutex);

    return d->addMimeTypes(device, errorMessage);
}

void QMimeDatabasePrivate::addGlobPattern(const QMimeGlobPattern& glob)
{
    m_mimeTypeGlobs.addGlob(glob);
}

void QMimeDatabaseBuilder::addGlobPattern(const QMimeGlobPattern& glob)
{
    d->addGlobPattern(glob);
}



/*!
    Returns a MIME type for \a typeOrAlias or Null one if none found.
*/
QMimeType QMimeDatabase::findByType(const QString &typeOrAlias) const
{
    QMutexLocker locker(&d->mutex);

    return d->findByType(typeOrAlias);
}

/*!
    Returns a MIME type for \a fileInfo or Null one if none found.
*/
QMimeType QMimeDatabase::findByFile(const QFileInfo &fileInfo) const
{
    QMutexLocker locker(&d->mutex);

    unsigned priority = 0;
    return d->findByFile(fileInfo, &priority);
}

/*!
    Returns a MIME type for the file \a name or Null one if none found.
    This function does not try to open the file. To determine the MIME type by its content, use
    QMimeDatabase::findByFile instead.
*/
QMimeType QMimeDatabase::findByName(const QString &name) const
{
    QMutexLocker locker(&d->mutex);

    QStringList matches = d->findByName(QFileInfo(name).fileName());
    const int matchCount = matches.count();
    if (matchCount == 0)
        return QMimeType();
    else if (matchCount == 1)
        return d->findByType(matches.first());
    else {
        // We have to pick one.
        matches.sort(); // Make it deterministic
        return d->findByType(matches.first());
    }
}

/*!
    Returns a MIME type for \a data or Null one if none found. This function reads content of a file
    and tries to determine it's type using magic sequencies.
*/
QMimeType QMimeDatabase::findByData(const QByteArray &data) const
{
    QMutexLocker locker(&d->mutex);

    unsigned priority = 0;
    return d->findByData(data, &priority);
}

void QMimeDatabaseBuilder::setMagicMatchers(const QString &typeOrAlias,
                                            const QList<QMimeMagicRuleMatcher> &matchers)
{
    QMutexLocker locker(&d->mutex);

    d->setMagicMatchers(typeOrAlias, matchers);
}

QList<QMimeType> QMimeDatabase::mimeTypes() const
{
    QMutexLocker locker(&d->mutex);

    return d->mimeTypes();
}

/*!
    Returns all known suffixes
*/
QStringList QMimeDatabaseBuilder::suffixes() const
{
    QMutexLocker locker(&d->mutex);

    return d->suffixes();
}

QString QMimeDatabaseBuilder::preferredSuffixByType(const QString &type) const
{
    d->mutex.lock();
    const QMimeType mt = d->findByType(type);
    d->mutex.unlock();
    if (mt.isValid())
        return mt.preferredSuffix(); // already does Mutex locking
    return QString();
}

QString QMimeDatabaseBuilder::preferredSuffixByFile(const QFileInfo &fileInfo) const
{
    d->mutex.lock();
    unsigned priority = 0;
    const QMimeType mt = d->findByFile(fileInfo, &priority);
    d->mutex.unlock();
    if (mt.isValid())
        return mt.preferredSuffix(); // already does Mutex locking
    return QString();
}

bool QMimeDatabaseBuilder::setPreferredSuffix(const QString &typeOrAlias, const QString &suffix)
{
    QMutexLocker locker(&d->mutex);

    return d->setPreferredSuffix(typeOrAlias, suffix);
}

QList<QMimeGlobPattern> QMimeDatabaseBuilder::toGlobPatterns(const QStringList &patterns, const QString &mimeType, int weight)
{
    return QMimeDatabasePrivate::toGlobPatterns(patterns, mimeType, weight);
}

QStringList QMimeDatabaseBuilder::fromGlobPatterns(const QList<QMimeGlobPattern> &globPatterns)
{
    return QMimeDatabasePrivate::fromGlobPatterns(globPatterns);
}

QStringList QMimeDatabaseBuilder::globPatterns() const
{
    QMutexLocker locker(&d->mutex);

    return fromGlobPatterns(d->globPatterns());
}

void QMimeDatabaseBuilder::setGlobPatterns(const QString &typeOrAlias,
                                           const QStringList &globPatterns)
{
    QMutexLocker locker(&d->mutex);

    d->setGlobPatterns(typeOrAlias, globPatterns);
}

QStringList QMimeDatabase::filterStrings() const
{
    QMutexLocker locker(&d->mutex);

    return d->filterStrings();
}

/*!
    Returns a string with all the possible file filters, for use with file dialogs
*/
QString QMimeDatabase::allFiltersString(QString *allFilesFilter) const
{
    if (allFilesFilter)
        allFilesFilter->clear();

    // Compile list of filter strings, sort, and remove duplicates (different MIME types might
    // generate the same filter).
    QStringList filters = filterStrings();
    if (filters.empty())
        return QString();
    filters.sort();
    filters.erase(std::unique(filters.begin(), filters.end()), filters.end());

    static const QString allFiles = QObject::tr("All Files (*)", "QMimeDatabase");
    if (allFilesFilter)
        *allFilesFilter = allFiles;

    // Prepend all files filter (instead of appending to work around a bug in Qt/Mac).
    filters.prepend(allFiles);

    return filters.join(QLatin1String(";;"));
}

QT_END_NAMESPACE
