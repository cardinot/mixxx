#include <QtDebug>

#include "effects/effectparameter.h"
#include "effects/effectsmanager.h"
#include "effects/effect.h"

EffectParameter::EffectParameter(Effect* pEffect, EffectsManager* pEffectsManager,
                                 int iParameterNumber, const EffectManifestParameter& parameter)
        : QObject(), // no parent
          m_pEffect(pEffect),
          m_pEffectsManager(pEffectsManager),
          m_iParameterNumber(iParameterNumber),
          m_parameter(parameter),
          m_bAddedToEngine(false) {
    // qDebug() << debugString() << "Constructing new EffectParameter from EffectManifestParameter:"
    //          << m_parameter.id();
     m_minimum = m_parameter.getMinimum();
     m_maximum = m_parameter.getMaximum();
     // Sanity check the maximum and minimum
     if (m_minimum > m_maximum) {
         qWarning() << debugString() << "WARNING: Parameter maximum is less than the minimum.";
         m_maximum = m_minimum;
     }

     // If the parameter specifies a default, set that. Otherwise use the minimum
     // value.
     m_default = m_parameter.getDefault();
     if (m_default < m_minimum || m_default > m_maximum) {
         qWarning() << debugString() << "WARNING: Parameter default is outside of minimum/maximum range.";
         m_default = m_minimum;
     }

     // Finally, set the value to the default.
     m_value = m_default;
}

EffectParameter::~EffectParameter() {
    //qDebug() << debugString() << "destroyed";
}

const EffectManifestParameter& EffectParameter::manifest() const {
    return m_parameter;
}

const QString EffectParameter::id() const {
    return m_parameter.id();
}

const QString EffectParameter::name() const {
    return m_parameter.name();
}

const QString EffectParameter::description() const {
    return m_parameter.description();
}

// static
bool EffectParameter::clampValue(double* pValue,
                                 const double& minimum, const double& maximum) {
    if (*pValue < minimum) {
        *pValue = minimum;
        return true;
    } else if (*pValue > maximum) {
        *pValue = maximum;
        return true;
    }
    return false;
}

bool EffectParameter::clampValue() {
    return clampValue(&m_value, m_minimum, m_maximum);
}

bool EffectParameter::clampDefault() {
    return clampValue(&m_default, m_minimum, m_maximum);
}

bool EffectParameter::clampRanges() {
    if (m_minimum > m_maximum) {
        m_maximum = m_minimum;
        return true;
    }
    return false;
}

EffectManifestParameter::LinkType EffectParameter::getDefaultLinkType() const {
    return m_parameter.defaultLinkType();
}

double EffectParameter::getNeutralPointOnScale() const {
    return m_parameter.neutralPointOnScale();
}

double EffectParameter::getValue() const {
    return m_value;
}

void EffectParameter::setValue(double value) {
    // TODO(XXX) Handle inf, -inf, and nan
    m_value = value;

    if (clampValue()) {
        qWarning() << debugString() << "WARNING: Value was outside of limits, clamped.";
    }

    m_value = value;

    updateEngineState();
    emit(valueChanged(m_value));
}

double EffectParameter::getDefault() const {
    return m_default;
}

void EffectParameter::setDefault(double dflt) {
    m_default = dflt;

    if (clampDefault()) {
        qWarning() << debugString() << "WARNING: Default parameter value was outside of range, clamped.";
    }

    m_default = dflt;

    updateEngineState();
}

double EffectParameter::getMinimum() const {
    return m_minimum;
}

void EffectParameter::setMinimum(double minimum) {
    m_minimum = minimum;

    if (m_minimum < m_parameter.getMinimum()) {
        qWarning() << debugString() << "WARNING: Minimum value is less than plugin's absolute minimum, clamping.";
        m_minimum = m_parameter.getMinimum();
    }

    if (m_minimum > m_maximum) {
        qWarning() << debugString() << "WARNING: New minimum was above maximum, clamped.";
        m_minimum = m_maximum;
    }

    // There's a degenerate case here where the maximum could be lower
    // than the manifest minimum. If that's the case, then the minimum
    // value is currently below the manifest minimum. Since similar
    // guards exist in the setMaximum call, this should not be able to
    // happen.
    Q_ASSERT(m_minimum >= m_parameter.getMinimum());

    if (clampValue()) {
        qWarning() << debugString() << "WARNING: Value was outside of new minimum, clamped.";
    }

    updateEngineState();
}

double EffectParameter::getMaximum() const {
    return m_maximum;
}

void EffectParameter::setMaximum(double maximum) {
    m_maximum = maximum;
    if (m_maximum > m_parameter.getMaximum()) {
        qWarning() << debugString() << "WARNING: Maximum value is less than plugin's absolute maximum, clamping.";
        m_maximum = m_parameter.getMaximum();
    }

    if (m_maximum < m_minimum) {
        qWarning() << debugString() << "WARNING: New maximum was below the minimum, clamped.";
        m_maximum = m_minimum;
    }

    // There's a degenerate case here where the minimum could be larger
    // than the manifest maximum. If that's the case, then the maximum
    // value is currently above the manifest maximum. Since similar
    // guards exist in the setMinimum call, this should not be able to
    // happen.
    Q_ASSERT(m_maximum <= m_parameter.getMaximum());

    if (clampValue()) {
        qWarning() << debugString() << "WARNING: Value was outside of new maximum, clamped.";
    }

    if (clampDefault()) {
        qWarning() << debugString() << "WARNING: Default was outside of new maximum, clamped.";
    }

    updateEngineState();
}

EffectManifestParameter::ControlHint EffectParameter::getControlHint() const {
    return m_parameter.controlHint();
}

void EffectParameter::updateEngineState() {
    EngineEffect* pEngineEffect = m_pEffect->getEngineEffect();
    if (!pEngineEffect) {
        return;
    }
    EffectsRequest* pRequest = new EffectsRequest();
    pRequest->type = EffectsRequest::SET_PARAMETER_PARAMETERS;
    pRequest->pTargetEffect = pEngineEffect;
    pRequest->SetParameterParameters.iParameter = m_iParameterNumber;
    pRequest->value = m_value;
    pRequest->minimum = m_minimum;
    pRequest->maximum = m_maximum;
    pRequest->default_value = m_default;
    m_pEffectsManager->writeRequest(pRequest);
}
