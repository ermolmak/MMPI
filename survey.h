#ifndef SURVEY_H
#define SURVEY_H

#include <cstddef>

#include <algorithm>
#include <exception>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QtGlobal>
#include <QVector>

class JsonReadException : public std::exception
{
public:
    JsonReadException()
        : msg() {}

    JsonReadException(const JsonReadException &) = default;

    explicit JsonReadException(const QString &message)
        : msg(message) {}

    JsonReadException &operator=(const JsonReadException &) = default;

    const char *what() const noexcept
    {
        return msg.toStdString().c_str();
    }

private:
    QString msg;
};

class Survey
{
public:
    struct Scale
    {
        // TODO: think about access rights
        void setFromJson(const QJsonObject &obj);

        QString name;
        double mean;
        double variance;
        double correction;
        QVector<std::size_t> statementsPositive;
        QVector<std::size_t> statementsNegative;

    private:
        static QVector<std::size_t> readStatementsNumbersFromJson(const QJsonObject &obj,
                                                                  const QString &memberName);
    };

    struct Score
    {
        QString scaleName;
        int initialScore;
        double tScore;
    };

    enum Answer {
        YES, NO, WITHOUT_ANSWER
    };

    void setFromJson(const QJsonObject &obj);
    QVector<Score> compute(QVector<Answer> answers);

private:
    static QVector<Scale> readScalesFromJson(const QJsonObject &obj, const QString &memberName);
    static QVector<QString> readStatementsFromJson(const QJsonObject &obj);

    QVector<Scale> primaryScales;
    QVector<Scale> additionalScales;
    QVector<QString> statements;
    int correctionScale;
};

#endif // SURVEY_H
