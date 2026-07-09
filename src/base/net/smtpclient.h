/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This code is based on QxtSmtp from libqxt (http://libqxt.org).
 */

#pragma once

#include <QAbstractSocket>
#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class QSslSocket;

namespace Net
{
    inline constexpr short SMTP_DEFAULT_PORT = 25;
    inline constexpr short SMTP_DEFAULT_PORT_SSL = 465;
    inline constexpr short SMTP_DEFAULT_PORT_STARTTLS = 587;

    /// Fire-and-forget SMTP sender used for the "email on download completion"
    /// feature. `sendMail()` constructs a self-owned client that connects using the
    /// mail settings from `Preferences`, negotiates auth/encryption, sends, and
    /// deletes itself. Every state transition/error is logged via `lcNet`.
    class SMTPClient final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(SMTPClient)

    public:
        /// Sends a single message asynchronously. `context`, when given, ties the
        /// client's lifetime to that object.
        static void sendMail(const QString &from, const QString &to
                , const QString &subject, const QString &body
                , QObject *context = nullptr);

    private slots:
        void readyRead();
        void error(QAbstractSocket::SocketError socketError);

    private:
        enum States
        {
            Rcpt,
            EhloSent,
            HeloSent,
            EhloDone,
            EhloGreetReceived,
            AuthRequestSent,
            AuthSent,
            AuthUsernameSent,
            Authenticated,
            StartTLSSent,
            Data,
            Init,
            Body,
            Quit,
            Close
        };

        enum AuthType
        {
            AuthPlain,
            AuthLogin,
            AuthCramMD5
        };

        SMTPClient(const QString &sender, const QStringList &recipients, const QString &subject
                   , const QString &body, QObject *parent = nullptr);
        ~SMTPClient() override;

        void ehlo();
        void helo();
        void parseEhloResponse(const QByteArray &code, bool continued, const QString &line);
        void authenticate();
        void startTLS();
        void authCramMD5(const QByteArray &challenge = {});
        void authPlain();
        void authLogin();
        void logError(const QString &msg);

        QByteArray m_message;
        QSslSocket *m_socket = nullptr;
        QString m_sender;
        QStringList m_recipients;
        QStringListIterator m_recipientsIterator;
        QString m_response;
        States m_state = Init;
        QHash<QString, QString> m_extensions;
        QByteArray m_buffer;
        bool m_usingStartTls = false;
        AuthType m_authType = AuthPlain;
        QString m_username;
        QString m_password;
    };
}
