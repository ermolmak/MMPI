#include "survey.h"

void Survey::Scale::setFromJson(const QJsonObject &obj, int statementsAmount)
{
    name = obj["name"].toString();
    if (name.isNull())
        throw JsonReadException("scale without a name");

    mean = obj["mean"].toDouble(-1);
    if (mean <= 0)
        throw JsonReadException("scale \"" + name + "\" has incorrect \"mean\" value");

    variance = obj["variance"].toDouble(-1);
    if (variance <= 0)
        throw JsonReadException("scale \"" + name + "\" has incorrect \"variance\" value");

    correction = obj["correction"].toDouble(qQNaN());
    if (qIsNaN(correction))
        correction = 0;

    try {
        statementsPositive = readStatementsNumbersFromJson(obj, "positive", statementsAmount);
        statementsNegative = readStatementsNumbersFromJson(obj, "negative", statementsAmount);
    } catch(JsonReadException e) {
        throw JsonReadException("scale \"" + name + "\": " + e.what());
    }
}

Survey::ScaleScore Survey::Scale::computeScale(const QVector<Answer> &answers,
                                               double correctionTScore) const
{
    ScaleScore result;
    result.scaleName = name;
    result.initialScore = 0;
    foreach (std::size_t i, statementsPositive) {
        if (answers[i] == Answer::YES)
            ++result.initialScore;
    }
    foreach (std::size_t i, statementsNegative) {
        if (answers[i] == Answer::NO)
            ++result.initialScore;
    }
    result.tScore = 50.0 + 10.0 * (result.initialScore - mean) / variance
                         + correctionTScore * correction;

    return result;
}

QVector<std::size_t> Survey::Scale::readStatementsNumbersFromJson(const QJsonObject &obj,
                                                                  const QString &memberName,
                                                                  int statementsAmount)
{
    QJsonValue value = obj[memberName];
    if (!value.isArray())
        throw JsonReadException("object doesn\'t contain an array named \"" + memberName + "\"");
    QJsonArray array = value.toArray();

    QVector<std::size_t> result;
    result.reserve(array.size());
    foreach (const QJsonValue &x, array) {
        int number = x.toInt(-1);
        if (number < 0 || number >= statementsAmount)
            throw JsonReadException("array named \"" + memberName + "\" has incorrect value");
        result.append(x.toInt());
    }

    return result;
}

void Survey::setFromJson(const QJsonObject &obj)
{
    QJsonValue descriptionValue = obj["description"];
    if (descriptionValue.isString())
        description = descriptionValue.toString();
    else
        throw JsonReadException("value \"description\" isn\'t found or"
                                "has wrong type (must be a string)");

    statements = readStatementsFromJson(obj);
    primaryScales = readScalesFromJson(obj, "primary_scales", statements.size());
    additionalScales = readScalesFromJson(obj, "additional_scales", statements.size());

    QJsonValue correction = obj["correction_scale"];
    if (correction.isString()) {
        QString name = correction.toString();

        auto iter = std::find_if(additionalScales.begin(),
                                 additionalScales.end(),
                                 [&name](const Scale &scale) { return scale.name == name; });

        if (iter == additionalScales.end())
            JsonReadException("scale \"" + name + "\" not found");

        correctionScale = iter - additionalScales.begin();
    } else if (correction.isNull()) {
        correctionScale = -1;
    } else {
        throw JsonReadException("value \"correction_scale\" isn\'t found or"
                                " has wrong type (must be a string or null)");
    }
}

Survey::TotalScore Survey::compute(const QVector<Answer> &answers) const
{
    if (answers.size() != statements.size()) {
        // TODO: exception
    }

    TotalScore result;
    result.unanswered = std::count(answers.begin(), answers.end(), Answer::WITHOUT_ANSWER);
    foreach (const Scale &scale, additionalScales) {
        result.scalesResults.append(scale.computeScale(answers));
    }
    foreach (const Scale &scale, primaryScales) {
        result.scalesResults.append(
                    scale.computeScale(answers, result.scalesResults[correctionScale].tScore));
    }

    return result;
}

QVector<Survey::Scale> Survey::readScalesFromJson(const QJsonObject &obj,
                                                  const QString &memberName,
                                                  int statementsAmount)
{
    QJsonValue valueScales = obj[memberName];
    if (!valueScales.isArray())
        JsonReadException("value \"" + memberName + "\" must be an array");

    QJsonArray scales = valueScales.toArray();
    QVector<Scale> result;
    result.reserve(scales.size());
    foreach (QJsonValue x, scales) {
        if (!x.isObject())
            JsonReadException("all values in the array \"" + memberName + "\" must be objects");

        Scale scale;
        try {
            scale.setFromJson(x.toObject(), statementsAmount);
        } catch (JsonReadException e) {
            throw JsonReadException(memberName + ": " + e.what());
        }
        result.append(scale);
    }

    return result;
}

QVector<QString> Survey::readStatementsFromJson(const QJsonObject &obj)
{
    QJsonValue valueStatements = obj["statements"];
    if (!valueStatements.isArray())
        JsonReadException("value \"statements\" must be an array");

    QJsonArray statements = valueStatements.toArray();
    QVector<QString> result;
    result.reserve(statements.size());
    foreach (QJsonValue x, statements) {
        if (!x.isString())
            JsonReadException("statements: all values must be strings");
        result.append(x.toString());
    }

    return result;
}
