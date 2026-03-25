/*
 * SPDX-FileCopyrightText: 2026 Badge Magic for SailfishOS contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Badge Magic for SailfishOS contributors
 *
 * Based on the original Badge Magic application by FOSSASIA.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QGuiApplication>
#include <QLocale>
#include <QQmlContext>
#include <QQuickView>
#include <QTranslator>
#include <sailfishapp.h>

#include "src/badgeapp.h"

namespace {

const char kTranslationBaseName[] = "harbour-badgemagic-sailfish";

void installAppTranslator(QGuiApplication *application)
{
    QTranslator *translator = new QTranslator(application);
    const QString translationsPath = SailfishApp::pathTo(QStringLiteral("translations")).toLocalFile();

    const bool loaded = translator->load(QLocale(), QString::fromLatin1(kTranslationBaseName),
                                         QStringLiteral("_"), translationsPath)
            || translator->load(QString::fromLatin1(kTranslationBaseName), translationsPath);

    if (loaded) {
        application->installTranslator(translator);
    } else {
        translator->deleteLater();
    }
}

}

int main(int argc, char *argv[])
{
    QGuiApplication *application = SailfishApp::application(argc, argv);
    application->setApplicationVersion(QStringLiteral(APP_VERSION));
    installAppTranslator(application);
    BadgeApp badgeApp;

    QQuickView *view = SailfishApp::createView();
    view->rootContext()->setContextProperty(QStringLiteral("badgeApp"), &badgeApp);
    view->rootContext()->setContextProperty(QStringLiteral("appVersion"), application->applicationVersion());
    view->setSource(SailfishApp::pathTo(QStringLiteral("qml/harbour-badgemagic-sailfish.qml")));
    view->show();

    return application->exec();
}
