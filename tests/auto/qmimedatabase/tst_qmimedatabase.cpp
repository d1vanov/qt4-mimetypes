/****************************************************************************
**
** TODO Provide Licensing information
**
** This file is part of the QtMimeTypes addon of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qmimedatabase.h"

#include <qmimedatabase.h>

#include "qstandardpaths.h"

#include <QtCore/QFile>

#include <QtTest/QtTest>

#if 0
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QtWidgets/QIcon>
#else
#include <QtGui/QIcon>
#endif
#endif

int initializeLang()
{
    qputenv("LANG", "en_US");
    return 0;
}

Q_CONSTRUCTOR_FUNCTION(initializeLang)

tst_qmimedatabase::~tst_qmimedatabase()
{
}

void tst_qmimedatabase::initTestCase()
{
    // Create a "global" and a "local" XDG data dir, right here.
    // The local dir will be empty initially, while the global dir will contain a copy of freedesktop.org.xml

    QDir here = QDir::currentPath();

    qputenv("XDG_DATA_DIRS", QFile::encodeName(here.absolutePath()));
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QDir(here.absolutePath() + "/mime").removeRecursively();
#endif
    here.mkpath(QString::fromLatin1("mime/packages"));

    QFile xml(QFile::decodeName(SRCDIR "../../../src/mimetypes/mime/packages/freedesktop.org.xml"));
    const QString mimeDir = here.absolutePath() + QLatin1String("/mime");
    xml.copy(mimeDir + QLatin1String("/packages/freedesktop.org.xml"));

    m_dataHome = here.absolutePath() + QLatin1String("/../datahome");
    qputenv("XDG_DATA_HOME", QFile::encodeName(m_dataHome));
    //qDebug() << "XDG_DATA_HOME=" << m_dataHome;

    // Make sure we start clean
    cleanupTestCase();
}

void tst_qmimedatabase::cleanupTestCase()
{
    QDir here = QDir::currentPath();
    here.remove(QString::fromLatin1("mime/packages/yast2-metapackage-handler-mimetypes.xml"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QDir(m_dataHome).removeRecursively();
#else
    QFile::remove(m_dataHome + QLatin1String("/mime/packages/yast2-metapackage-handler-mimetypes.xml"));
#endif
}

void tst_qmimedatabase::test_mimeTypeForName()
{
    QMimeDatabase db;
    QMimeType s0 = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QVERIFY(s0.isValid());
    QCOMPARE(s0.name(), QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0.comment(), QString::fromLatin1("empty document"));
    QCOMPARE(s0.comment(QLatin1String("fr")), QString::fromLatin1("document vide"));
    QCOMPARE(s0.comment(QLatin1String("zz")), QString::fromLatin1("empty document")); // no such language

    QMimeType s0Again = db.mimeTypeForName(QString::fromLatin1("application/x-zerosize"));
    QCOMPARE(s0Again.name(), s0.name());

    QMimeType s1 = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    QVERIFY(s1.isValid());
    QCOMPARE(s1.name(), QString::fromLatin1("text/plain"));
    //qDebug("Comment is %s", qPrintable(s1.comment()));

    QMimeType krita = db.mimeTypeForName(QString::fromLatin1("application/x-krita"));
    QVERIFY(krita.isValid());

    // Test <comment> parsing with application/rdf+xml which has the english comment after the other ones
    QMimeType rdf = db.mimeTypeForName(QString::fromLatin1("application/rdf+xml"));
    QVERIFY(rdf.isValid());
    QCOMPARE(rdf.comment(), QString::fromLatin1("RDF file"));

    QMimeType bzip2 = db.mimeTypeForName(QString::fromLatin1("application/x-bzip2"));
    QVERIFY(bzip2.isValid());
    QCOMPARE(bzip2.comment(), QString::fromLatin1("Bzip archive"));

    QMimeType defaultMime = db.mimeTypeForName(QString::fromLatin1("application/octet-stream"));
    QVERIFY(defaultMime.isValid());
    QVERIFY(defaultMime.isDefault());

    QMimeType doesNotExist = db.mimeTypeForName(QString::fromLatin1("foobar/x-doesnot-exist"));
    QVERIFY(!doesNotExist.isValid());

    // TODO move to test_findByFile
#ifdef Q_OS_LINUX
    QString exePath = QStandardPaths::findExecutable(QLatin1String("ls"));
    if (exePath.isEmpty())
        qWarning() << "ls not found";
    else {
        const QString executableType = QString::fromLatin1("application/x-executable");
        //QTest::newRow("executable") << exePath << executableType;
        QCOMPARE(db.mimeTypeForFile(exePath).name(), executableType);
    }
#endif

}

void tst_qmimedatabase::test_mimeTypeForFileName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedMimeType");

    QTest::newRow("text") << "textfile.txt" << "text/plain";
    QTest::newRow("case-insensitive search") << "textfile.TxT" << "text/plain";

    // Needs shared-mime-info > 0.91. Earlier versions wrote .Z to the mime.cache file...
    //QTest::newRow("case-insensitive match on a non-lowercase glob") << "foo.z" << "application/x-compress";

    QTest::newRow("case-sensitive uppercase match") << "textfile.C" << "text/x-c++src";
    QTest::newRow("case-sensitive lowercase match") << "textfile.c" << "text/x-csrc";
    QTest::newRow("case-sensitive long-extension match") << "foo.PS.gz" << "application/x-gzpostscript";
    QTest::newRow("case-sensitive-only match") << "core" << "application/x-core";
    QTest::newRow("case-sensitive-only match") << "Core" << "application/octet-stream"; // #198477

    QTest::newRow("desktop file") << "foo.desktop" << "application/x-desktop";
    QTest::newRow("old kdelnk file is x-desktop too") << "foo.kdelnk" << "application/x-desktop";
    QTest::newRow("double-extension file") << "foo.tar.bz2" << "application/x-bzip-compressed-tar";
    QTest::newRow("single-extension file") << "foo.bz2" << "application/x-bzip";
    QTest::newRow(".doc should assume msword") << "somefile.doc" << "application/msword"; // #204139
    QTest::newRow("glob that uses [] syntax, 1") << "Makefile" << "text/x-makefile";
    QTest::newRow("glob that uses [] syntax, 2") << "makefile" << "text/x-makefile";
    QTest::newRow("glob that ends with *, no extension") << "README" << "text/x-readme";
    QTest::newRow("glob that ends with *, extension") << "README.foo" << "text/x-readme";
    QTest::newRow("glob that ends with *, also matches *.txt. Higher weight wins.") << "README.txt" << "text/plain";
    QTest::newRow("glob that ends with *, also matches *.nfo. Higher weight wins.") << "README.nfo" << "text/x-nfo";
    // fdo bug 15436, needs shared-mime-info >= 0.40 (and this tests the globs2-parsing code).
    QTest::newRow("glob that ends with *, also matches *.pdf. *.pdf has higher weight") << "README.pdf" << "application/pdf";
    QTest::newRow("directory") << "/" << "inode/directory";
    QTest::newRow("doesn't exist, no extension") << "IDontExist" << "application/octet-stream";
    QTest::newRow("doesn't exist but has known extension") << "IDontExist.txt" << "text/plain";
}

void tst_qmimedatabase::test_mimeTypeForFileName()
{
    QFETCH(QString, fileName);
    QFETCH(QString, expectedMimeType);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QCOMPARE(mime.name(), expectedMimeType);

    QList<QMimeType> mimes = db.findMimeTypesByFileName(fileName);
    if (expectedMimeType == "application/octet-stream") {
        QVERIFY(mimes.isEmpty());
    } else {
        QVERIFY(!mimes.isEmpty());
        QCOMPARE(mimes.count(), 1);
        QCOMPARE(mimes.first().name(), expectedMimeType);
    }
}

void tst_qmimedatabase::test_findMultipleByName_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QStringList>("expectedMimeTypes");

    QTest::newRow("txt, 1 hit") << "foo.txt" << (QStringList() << "text/plain");
    QTest::newRow("txtfoobar, 0 hit") << "foo.foobar" << QStringList();
    QTest::newRow("m, 2 hits") << "foo.m" << (QStringList() << "text/x-matlab" << "text/x-objcsrc");
    QTest::newRow("sub, 3 hits") << "foo.sub" << (QStringList() << "text/x-microdvd" << "text/x-mpsub" << "text/x-subviewer");
}

void tst_qmimedatabase::test_findMultipleByName()
{
    QFETCH(QString, fileName);
    QFETCH(QStringList, expectedMimeTypes);
    QMimeDatabase db;
    QList<QMimeType> mimes = db.findMimeTypesByFileName(fileName);
    QStringList mimeNames;
    foreach (const QMimeType& mime, mimes)
        mimeNames.append(mime.name());
    QCOMPARE(mimeNames, expectedMimeTypes);
}

void tst_qmimedatabase::test_inheritance()
{
    QMimeDatabase db;

    // All file-like mimetypes inherit from octet-stream
    const QMimeType wordperfect = db.mimeTypeForName(QString::fromLatin1("application/vnd.wordperfect"));
    QVERIFY(wordperfect.isValid());
    QCOMPARE(wordperfect.parentMimeTypes().join(QString::fromLatin1(",")), QString::fromLatin1("application/octet-stream"));
    QVERIFY(wordperfect.inherits(QLatin1String("application/octet-stream")));

    QVERIFY(db.mimeTypeForName(QString::fromLatin1("image/svg+xml-compressed")).inherits(QLatin1String("application/x-gzip")));

    // Check that msword derives from ole-storage
    const QMimeType msword = db.mimeTypeForName(QString::fromLatin1("application/msword"));
    QVERIFY(msword.isValid());
    const QMimeType olestorage = db.mimeTypeForName(QString::fromLatin1("application/x-ole-storage"));
    QVERIFY(olestorage.isValid());
    QVERIFY(msword.inherits(olestorage.name()));
    QVERIFY(msword.inherits(QLatin1String("application/octet-stream")));

    const QMimeType directory = db.mimeTypeForName(QString::fromLatin1("inode/directory"));
    QVERIFY(directory.isValid());
    QCOMPARE(directory.parentMimeTypes().count(), 0);
    QVERIFY(!directory.inherits(QLatin1String("application/octet-stream")));

    // Check that text/x-patch knows that it inherits from text/plain (it says so explicitly)
    const QMimeType plain = db.mimeTypeForName(QString::fromLatin1("text/plain"));
    const QMimeType derived = db.mimeTypeForName(QString::fromLatin1("text/x-patch"));
    QVERIFY(derived.isValid());
    QCOMPARE(derived.parentMimeTypes().join(QString::fromLatin1(",")), plain.name());
    QVERIFY(derived.inherits(QLatin1String("text/plain")));
    QVERIFY(derived.inherits(QLatin1String("application/octet-stream")));

    // Check that application/x-shellscript inherits from application/x-executable
    // (Otherwise KRun cannot start shellscripts...)
    // This is a test for multiple inheritance...
    const QMimeType shellscript = db.mimeTypeForName(QString::fromLatin1("application/x-shellscript"));
    QVERIFY(shellscript.isValid());
    QVERIFY(shellscript.inherits(QLatin1String("text/plain")));
    QVERIFY(shellscript.inherits(QLatin1String("application/x-executable")));
    const QStringList shellParents = shellscript.parentMimeTypes();
    QVERIFY(shellParents.contains(QLatin1String("text/plain")));
    QVERIFY(shellParents.contains(QLatin1String("application/x-executable")));
    QCOMPARE(shellParents.count(), 2); // only the above two
    const QStringList allShellParents = shellscript.allParentMimeTypes();
    QVERIFY(allShellParents.contains(QLatin1String("text/plain")));
    QVERIFY(allShellParents.contains(QLatin1String("application/x-executable")));
    QVERIFY(allShellParents.contains(QLatin1String("application/octet-stream")));
    // Must be least-specific last, i.e. breadth first.
    QCOMPARE(allShellParents.last(), QString::fromLatin1("application/octet-stream"));

    const QStringList allSvgParents = db.mimeTypeForName(QString::fromLatin1("image/svg+xml")).allParentMimeTypes();
    QCOMPARE(allSvgParents, QStringList() << QLatin1String("application/xml") << QLatin1String("text/plain") << QLatin1String("application/octet-stream"));

    // Check that text/x-mrml knows that it inherits from text/plain (implicitly)
    const QMimeType mrml = db.mimeTypeForName(QString::fromLatin1("text/x-mrml"));
    QVERIFY(mrml.isValid());
    QVERIFY(mrml.inherits(QLatin1String("text/plain")));
    QVERIFY(mrml.inherits(QLatin1String("application/octet-stream")));

    // Check that msword-template inherits msword
    const QMimeType mswordTemplate = db.mimeTypeForName(QString::fromLatin1("application/msword-template"));
    QVERIFY(mswordTemplate.isValid());
    QVERIFY(mswordTemplate.inherits(QLatin1String("application/msword")));
}

void tst_qmimedatabase::test_aliases()
{
    QMimeDatabase db;

    const QMimeType canonical = db.mimeTypeForName(QString::fromLatin1("application/xml"));
    QVERIFY(canonical.isValid());

    QMimeType resolvedAlias = db.mimeTypeForName(QString::fromLatin1("text/xml"));
    QVERIFY(resolvedAlias.isValid());
    QCOMPARE(resolvedAlias.name(), QString::fromLatin1("application/xml"));

    QVERIFY(resolvedAlias.inherits(QLatin1String("application/xml")));
    QVERIFY(canonical.inherits(QLatin1String("text/xml")));

    // Test for kde bug 197346: does nspluginscan see that audio/mp3 already exists?
    bool mustWriteMimeType = !db.mimeTypeForName(QString::fromLatin1("audio/mp3")).isValid();
    QVERIFY(!mustWriteMimeType);
}

void tst_qmimedatabase::test_icons()
{
    QMimeDatabase db;
    QMimeType directory = db.mimeTypeForFile(QString::fromLatin1("/"));
    QCOMPARE(directory.name(), QString::fromLatin1("inode/directory"));
    QCOMPARE(directory.iconName(), QString::fromLatin1("inode-directory"));
    QCOMPARE(directory.genericIconName(), QString::fromLatin1("inode-x-generic"));

    QMimeType pub = db.mimeTypeForFile(QString::fromLatin1("foo.epub"), QMimeDatabase::MatchExtension);
    QCOMPARE(pub.name(), QString::fromLatin1("application/epub+zip"));
    QCOMPARE(pub.iconName(), QString::fromLatin1("application-epub+zip"));
    QCOMPARE(pub.genericIconName(), QString::fromLatin1("x-office-document"));
}

// In here we do the tests that need some content in a temporary file.
// This could also be added to shared-mime-info's testsuite...
void tst_qmimedatabase::test_findByFileWithContent()
{
    QMimeDatabase db;
    QMimeType mime;

    // Test a real PDF file.
    // If we find x-matlab because it starts with '%' then we are not ordering by priority.
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QString tempFileName = tempFile.fileName();
    tempFile.write("%PDF-");
    tempFile.close();
    mime = db.mimeTypeForFile(tempFileName);
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    QFile file(tempFileName);
    mime = db.findByData(&file); // QIODevice ctor
    QCOMPARE(mime.name(), QString::fromLatin1("application/pdf"));
    // by name only, we cannot find the mimetype
    mime = db.mimeTypeForFile(tempFileName, QMimeDatabase::MatchExtension);
    QVERIFY(mime.isValid());
    QVERIFY(mime.isDefault());

    // Test the case where the extension doesn't match the contents: extension wins
    {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_qmimedatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("%PDF-");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
        // fast mode finds the same
        mime = db.mimeTypeForFile(txtTempFileName, QMimeDatabase::MatchExtension);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
    }

    // Now the case where extension differs from contents, but contents has >80 magic rule
    // XDG spec says: contents wins. But we can't sniff all files...
    {
        QTemporaryFile txtTempFile(QDir::tempPath() + QLatin1String("/tst_qmimedatabase_XXXXXX.txt"));
        QVERIFY(txtTempFile.open());
        txtTempFile.write("<smil");
        QString txtTempFileName = txtTempFile.fileName();
        txtTempFile.close();
        mime = db.mimeTypeForFile(txtTempFileName);
        QCOMPARE(mime.name(), QString::fromLatin1("text/plain"));
    }

    // Test what happens with an incorrect path
    mime = db.mimeTypeForFile(QString::fromLatin1("file:///etc/passwd" /* incorrect code, use a path instead */));
    QVERIFY(mime.isDefault());

    // findByData when the device cannot be opened (e.g. a directory)
    QFile dir("/");
    mime = db.findByData(&dir);
    QVERIFY(mime.isDefault());
}

void tst_qmimedatabase::test_findByUrl()
{
    QMimeDatabase db;
    QVERIFY(db.findByUrl(QUrl::fromEncoded("http://foo/bar.png")).isDefault()); // HTTP can't know before downloading
    QCOMPARE(db.findByUrl(QUrl::fromEncoded("ftp://foo/bar.png")).name(), QString::fromLatin1("image/png"));
    QCOMPARE(db.findByUrl(QUrl::fromEncoded("ftp://foo/bar")).name(), QString::fromLatin1("application/octet-stream")); // unknown extension
}

void tst_qmimedatabase::test_findByContent_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("tnef data, needs smi >= 0.20") << QByteArray("\x78\x9f\x3e\x22") << "application/vnd.ms-tnef";
    QTest::newRow("PDF magic") << QByteArray("%PDF-") << "application/pdf";
    QTest::newRow("PHP, High-priority rule") << QByteArray("<?php") << "application/x-php";
    QTest::newRow("unknown") << QByteArray("\001abc?}") << "application/octet-stream";
}

void tst_qmimedatabase::test_findByContent()
{
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.findByData(data).name(), expectedMimeTypeName);
    QBuffer buffer(&data);
    QCOMPARE(db.findByData(&buffer).name(), expectedMimeTypeName);
}

void tst_qmimedatabase::test_mimeTypeForFileAndContent_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QString>("expectedMimeTypeName");

    QTest::newRow("plain text, no extension") << QString::fromLatin1("textfile") << QByteArray("Hello world") << "text/plain";
    QTest::newRow("plain text, unknown extension") << QString::fromLatin1("textfile.foo") << QByteArray("Hello world") << "text/plain";
    // Needs kde/mimetypes.xml
    //QTest::newRow("plain text, doc extension") << QString::fromLatin1("textfile.doc") << QByteArray("Hello world") << "text/plain";

    // If you get powerpoint instead, then you're hit by https://bugs.freedesktop.org/show_bug.cgi?id=435,
    // upgrade to shared-mime-info >= 0.22
    const QByteArray oleData("\320\317\021\340\241\261\032\341"); // same as \xD0\xCF\x11\xE0 \xA1\xB1\x1A\xE1
    QTest::newRow("msword file, unknown extension") << QString::fromLatin1("mswordfile") << oleData << "application/x-ole-storage";
    QTest::newRow("excel file, found by extension") << QString::fromLatin1("excelfile.xls") << oleData << "application/vnd.ms-excel";
    QTest::newRow("text.xls, found by extension, user is in control") << QString::fromLatin1("text.xls") << oleData << "application/vnd.ms-excel";
}

void tst_qmimedatabase::test_mimeTypeForFileAndContent()
{
    QFETCH(QString, name);
    QFETCH(QByteArray, data);
    QFETCH(QString, expectedMimeTypeName);

    QMimeDatabase db;
    QCOMPARE(db.mimeTypeForNameAndData(name, data).name(), expectedMimeTypeName);
}

void tst_qmimedatabase::test_allMimeTypes()
{
    QMimeDatabase db;
    const QList<QMimeType> lst = db.allMimeTypes(); // does NOT include aliases
    QVERIFY(!lst.isEmpty());

    // Hardcoding this is the only way to check both providers find the same number of mimetypes.
    QCOMPARE(lst.count(), 660);

    foreach (const QMimeType& mime, lst) {
        const QString name = mime.name();
        QVERIFY(!name.isEmpty());
        QCOMPARE(name.count(QLatin1Char('/')), 1);
        const QMimeType lookedupMime = db.mimeTypeForName(name);
        QVERIFY(lookedupMime.isValid());
        QCOMPARE(lookedupMime.name(), name); // if this fails, you have an alias defined as a real mimetype too!
    }
}

void tst_qmimedatabase::test_inheritsPerformance()
{
    // Check performance of inherits().
    // This benchmark (which started in 2009 in kmimetypetest.cpp) uses 40 mimetypes.
    QStringList mimeTypes; mimeTypes << QLatin1String("image/jpeg") << QLatin1String("image/png") << QLatin1String("image/tiff") << QLatin1String("text/plain") << QLatin1String("text/html");
    mimeTypes += mimeTypes;
    mimeTypes += mimeTypes;
    mimeTypes += mimeTypes;
    QCOMPARE(mimeTypes.count(), 40);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(QString::fromLatin1("text/x-chdr"));
    QVERIFY(mime.isValid());
    QBENCHMARK {
        QString match;
        foreach (const QString& mt, mimeTypes) {
            if (mime.inherits(mt)) {
                match = mt;
                // of course there would normally be a "break" here, but we're testing worse-case
                // performance here
            }
        }
        QCOMPARE(match, QString::fromLatin1("text/plain"));
    }
    // Numbers from 2011, in release mode:
    // KDE 4.7 numbers: 0.21 msec / 494,000 ticks / 568,345 instr. loads per iteration
    // QMimeBinaryProvider (with Qt 5): 0.16 msec / NA / 416,049 instr. reads per iteration
    // QMimeXmlProvider (with Qt 5): 0.062 msec / NA / 172,889 instr. reads per iteration
    //   (but the startup time is way higher)
    // And memory usage is flat at 200K with QMimeBinaryProvider, while it peaks at 6 MB when
    // parsing XML, and then keeps being around 4.5 MB for all the in-memory hashes.
}

void tst_qmimedatabase::test_suffixes_data()
{
    QTest::addColumn<QString>("mimeType");
    QTest::addColumn<QString>("patterns");
    QTest::addColumn<QString>("preferredSuffix");

    QTest::newRow("mimetype with a single pattern") << "application/pdf" << "*.pdf" << "pdf";
    QTest::newRow("mimetype with multiple patterns") << "application/x-kpresenter" << "*.kpr;*.kpt" << "kpr";
    //if (KMimeType::sharedMimeInfoVersion() > KDE_MAKE_VERSION(0, 60, 0)) {
        QTest::newRow("mimetype with many patterns") << "application/vnd.wordperfect" << "*.wp;*.wp4;*.wp5;*.wp6;*.wpd;*.wpp" << "wp";
    //}
    QTest::newRow("oasis text mimetype") << "application/vnd.oasis.opendocument.text" << "*.odt" << "odt";
    QTest::newRow("oasis presentation mimetype") << "application/vnd.oasis.opendocument.presentation" << "*.odp" << "odp";
    QTest::newRow("mimetype with multiple patterns") << "text/plain" << "*.asc;*.txt;*,v" << "txt";
    QTest::newRow("mimetype with uncommon pattern") << "text/x-readme" << "README*" << QString();
    QTest::newRow("mimetype with no patterns") << "application/pkcs7-mime" << QString() << QString();
}

void tst_qmimedatabase::test_suffixes()
{
    QFETCH(QString, mimeType);
    QFETCH(QString, patterns);
    QFETCH(QString, preferredSuffix);
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(mimeType);
    QVERIFY(mime.isValid());
    // Sort both lists; order is unreliable since shared-mime-info uses hashes internally.
    QStringList expectedPatterns = patterns.split(QLatin1Char(';'));
    expectedPatterns.sort();
    QStringList mimePatterns = mime.globPatterns();
    mimePatterns.sort();
    QCOMPARE(mimePatterns.join(QLatin1String(";")), expectedPatterns.join(QLatin1String(";")));
    QCOMPARE(mime.preferredSuffix(), preferredSuffix);
}

void tst_qmimedatabase::test_knownSuffix()
{
    QMimeDatabase db;
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar")), QString::fromLatin1("tar"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.bar.bz2")), QString::fromLatin1("bz2"));
    QCOMPARE(db.suffixForFileName(QString::fromLatin1("foo.tar.bz2")), QString::fromLatin1("tar.bz2"));
}

void tst_qmimedatabase::findByFileName_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QString>("mimeTypeName");
    QTest::addColumn<QString>("xFail");

    QString prefix = QLatin1String(SRCDIR "testfiles/");

    QFile f(prefix + QLatin1String("list"));
    QVERIFY2(f.open(QIODevice::ReadOnly), qPrintable(prefix));

    QByteArray line(1024, Qt::Uninitialized);

    while (!f.atEnd()) {
        int len = f.readLine(line.data(), 1023);

        if (len <= 2 || line.at(0) == '#')
            continue;

        QString string = QString::fromLatin1(line.constData(), len - 1).trimmed();
        QStringList list = string.split(QLatin1Char(' '), QString::SkipEmptyParts);
        QVERIFY(list.size() >= 2);

        QString filePath = list.at(0);
        QString mimeTypeType = list.at(1);
        QString xFail;
        if (list.size() >= 3)
            xFail = list.at(2);

        QTest::newRow(filePath.toLatin1().constData()) << QString(prefix + filePath) << mimeTypeType << xFail;
    }
}

void tst_qmimedatabase::findByFileName()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;

    //qDebug() << Q_FUNC_INFO << filePath;

    const QMimeType resultMimeType(database.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension));
    if (resultMimeType.isValid()) {
        //qDebug() << Q_FUNC_INFO << "MIME type" << resultMimeType.name() << "has generic icon name" << resultMimeType.genericIconName() << "and icon name" << resultMimeType.iconName();

// Loading icons depend on the icon theme, we can't enable this test
#if 0 // (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
        QCOMPARE(resultMimeType.genericIconName(), QIcon::fromTheme(resultMimeType.genericIconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.genericIconName()).isNull(), qPrintable(resultMimeType.genericIconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.genericIconName()), qPrintable(resultMimeType.genericIconName()));

        QCOMPARE(resultMimeType.iconName(), QIcon::fromTheme(resultMimeType.iconName()).name());
        QVERIFY2(!QIcon::fromTheme(resultMimeType.iconName()).isNull(), qPrintable(resultMimeType.iconName()));
        QVERIFY2(QIcon::hasThemeIcon(resultMimeType.iconName()), qPrintable(resultMimeType.iconName()));
#else
        // Under Qt4 not all genericIconNames or iconNames return an icon that is valid.
#endif
    }
    const QString resultMimeTypeName = resultMimeType.name();
    //qDebug() << Q_FUNC_INFO << "mimeTypeForFile() returned" << resultMimeTypeName;

    const bool failed = resultMimeTypeName != mimeTypeName;
    const bool shouldFail = (xFail.length() >= 1 && xFail.at(0) == QLatin1Char('x'));
    if (shouldFail != failed) {
        // Results are ambiguous when multiple MIME types have the same glob
        // -> accept the current result if the found MIME type actually
        // matches the file's extension.
        // TODO: a better file format in testfiles/list!
        const QMimeType foundMimeType = database.mimeTypeForName(resultMimeTypeName);
        QVERIFY2(resultMimeType == foundMimeType, qPrintable(resultMimeType.name() + QString::fromLatin1(" vs. ") + foundMimeType.name()));
        if (foundMimeType.isValid()) {
            const QString extension = QFileInfo(filePath).suffix();
            //qDebug() << Q_FUNC_INFO << "globPatterns:" << foundMimeType.globPatterns() << "- extension:" << QString() + "*." + extension;
            if (foundMimeType.globPatterns().contains(QString::fromLatin1("*.") + extension))
                return;
        }
    }
    if (shouldFail) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath), QMimeDatabase::MatchExtension);
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);
}

void tst_qmimedatabase::findByData_data()
{
    findByFileName_data();
}

void tst_qmimedatabase::findByData()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QByteArray data = f.read(16384);

    const QString resultMimeTypeName = database.findByData(data).name();
    if (xFail.length() >= 2 && xFail.at(1) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    QFileInfo info(filePath);
    QString mimeForInfo = database.mimeTypeForFile(info, QMimeDatabase::MatchContent).name();
    QCOMPARE(mimeForInfo, resultMimeTypeName);
}

void tst_qmimedatabase::findByFile_data()
{
    findByFileName_data();
}

void tst_qmimedatabase::findByFile()
{
    QFETCH(QString, filePath);
    QFETCH(QString, mimeTypeName);
    QFETCH(QString, xFail);

    QMimeDatabase database;
    const QString resultMimeTypeName = database.mimeTypeForFile(filePath).name();
    //qDebug() << Q_FUNC_INFO << filePath << "->" << resultMimeTypeName;
    if (xFail.length() >= 3 && xFail.at(2) == QLatin1Char('x')) {
        // Expected to fail
        QVERIFY2(resultMimeTypeName != mimeTypeName, qPrintable(resultMimeTypeName));
    } else {
        QCOMPARE(resultMimeTypeName, mimeTypeName);
    }

    // Test QFileInfo overload
    const QMimeType mimeForFileInfo = database.mimeTypeForFile(QFileInfo(filePath));
    QCOMPARE(mimeForFileInfo.name(), resultMimeTypeName);
}


void tst_qmimedatabase::test_fromThreads()
{
    // When using -Wshadow, a Qt 4 header breaks
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QThreadPool::globalInstance()->setMaxThreadCount(20);
    // Note that data-based tests cannot be used here (QTest::fetchData asserts).
    QList<QFuture<void> > futures;
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_mimeTypeForName);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_aliases);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_allMimeTypes);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_icons);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_inheritance);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_knownSuffix);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_findByFileWithContent);
    futures << QtConcurrent::run(this, &tst_qmimedatabase::test_allMimeTypes); // a second time
    Q_FOREACH(QFuture<void> f, futures) // krazy:exclude=foreach
        f.waitForFinished();
#endif
}

static void runUpdateMimeDatabase(const QString &path) // TODO make it a QMimeDatabase method?
{
    const QString umd = QStandardPaths::findExecutable(QString::fromLatin1("update-mime-database"));
    if (umd.isEmpty())
        QSKIP("shared-mime-info not found, skipping mime.cache test", SkipAll);

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); // silence output
    proc.start(umd, QStringList() << path);
    proc.waitForFinished();
    //qDebug() << "runUpdateMimeDatabase" << path;
}

static void waitAndRunUpdateMimeDatabase(const QString &path)
{
    QFileInfo mimeCacheInfo(path + QString::fromLatin1("/mime.cache"));
    if (mimeCacheInfo.exists()) {
        // Wait until the begining of the next second
        while (mimeCacheInfo.lastModified().secsTo(QDateTime::currentDateTime()) == 0) {
            QTest::qSleep(200);
        }
    }
    runUpdateMimeDatabase(path);
}

static void checkHasMimeType(const QString &mimeType)
{
    QMimeDatabase db;
    QVERIFY(db.mimeTypeForName(mimeType).isValid());

    bool found = false;
    foreach (const QMimeType& mt, db.allMimeTypes()) {
        if (mt.name() == mimeType) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

extern QMIME_EXPORT int qmime_secondsBetweenChecks; // see qmimeprovider.cpp

void tst_qmimedatabase::installNewGlobalMimeType()
{
    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    const QString fileName = QLatin1String("yast2-metapackage-handler-mimetypes.xml");
    const QString srcFile = QFile::decodeName(SRCDIR) + fileName;

    QDir here = QDir::currentPath();
    const QString mimeDir = here.absolutePath() + QLatin1String("/mime");
    const QString destDir = mimeDir + QLatin1String("/packages/");
    const QString destFile = destDir + fileName;
    QFile::remove(destFile);
    //qDebug() << destFile;
    QVERIFY(QFile::copy(srcFile, destFile));
    waitAndRunUpdateMimeDatabase(mimeDir);

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-suse-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    checkHasMimeType("text/x-suse-ymp");

    // Now test removing it again
    QFile::remove(destFile);
    waitAndRunUpdateMimeDatabase(mimeDir);
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
}

void tst_qmimedatabase::installNewLocalMimeType()
{
    qmime_secondsBetweenChecks = 0;

    QMimeDatabase db;
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    const QString fileName = QLatin1String("yast2-metapackage-handler-mimetypes.xml");
    const QString srcFile = QFile::decodeName(SRCDIR) + fileName;
    const QString mimeDir = m_dataHome + QLatin1String("/mime");
    const QString destDir = mimeDir + QLatin1String("/packages/");
    QDir().mkpath(destDir);
    const QString destFile = destDir + fileName;
    QFile::remove(destFile);
    QVERIFY(QFile::copy(srcFile, destFile));
    runUpdateMimeDatabase(mimeDir);

    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("text/x-suse-ymu"));
    QVERIFY(db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
    checkHasMimeType("text/x-suse-ymp");

    // Now test removing it again (note, this leaves a mostly-empty mime.cache file)
    QFile::remove(destFile);
    waitAndRunUpdateMimeDatabase(mimeDir);
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());

    // And now the user goes wild and uses rm -rf
    QFile::remove(mimeDir + QString::fromLatin1("/mime.cache"));
    QCOMPARE(db.mimeTypeForFile(QLatin1String("foo.ymu"), QMimeDatabase::MatchExtension).name(),
             QString::fromLatin1("application/octet-stream"));
    QVERIFY(!db.mimeTypeForName(QLatin1String("text/x-suse-ymp")).isValid());
}

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
//QTEST_MAIN(tst_qmimedatabase)
QTEST_GUILESS_MAIN(tst_qmimedatabase)
#else
// If tests with icons were activated in Qt4 we'd use QTEST_MAIN:
//QTEST_MAIN(tst_qmimedatabase)
    QTEST_MAIN(tst_qmimedatabase)
#endif
