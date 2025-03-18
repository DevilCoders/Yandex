
package ru.yandex.ci.client.sandbox.api;

import java.io.Serializable;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nullable;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlType;

import org.apache.commons.lang3.builder.ReflectionToStringBuilder;
import org.apache.commons.lang3.builder.ToStringStyle;



/**
 * <p>Java class for SandboxCustomField complex type.
 *
 * <p>The following schema fragment specifies the expected content contained within this class.
 *
 * <pre>
 * &lt;complexType name="SandboxCustomField">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;sequence>
 *         &lt;element name="name" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="value" type="{http://www.w3.org/2001/XMLSchema}anyType"/>
 *       &lt;/sequence>
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "SandboxCustomField", propOrder = {
        "name",
        "value"
})
@XmlRootElement(name = "sandboxCustomField")
@SuppressWarnings("EqualsGetClass")
public class SandboxCustomField implements Serializable {

    @XmlElement(required = true)
    protected String name;

    @XmlElement(required = true)
    protected Object value;

    /**
     * Default no-arg constructor
     */
    public SandboxCustomField() {
        super();
    }

    /**
     * Fully-initialising value constructor
     */
    public SandboxCustomField(final String name, final String value) {
        this.name = name;
        this.value = value;
    }

    public SandboxCustomField(final String name, final Number value) {
        this.name = name;
        this.value = value;
    }

    public SandboxCustomField(final String name, final Boolean value) {
        this.name = name;
        this.value = value;
    }

    public SandboxCustomField(final String name, final Map<String, Object> value) {
        this.name = name;
        this.value = value;
    }

    public SandboxCustomField(final String name, final List<Object> value) {
        this.name = name;
        this.value = value;
    }

    /**
     * Gets the value of the name property.
     *
     * @return possible object is
     * {@link String }
     */
    public String getName() {
        return name;
    }

    /**
     * Sets the value of the name property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setName(String value) {
        this.name = value;
    }

    /**
     * Gets the value of the value property.
     *
     * @return possible object is
     * {@link Object }
     */
    public Object getValue() {
        return value;
    }

    /**
     * Sets the value of the value property.
     *
     * @param value allowed object is
     *              {@link Object }
     */
    public void setValue(Object value) {
        this.value = value;
    }

    public SandboxCustomField withName(String value) {
        setName(value);
        return this;
    }

    public SandboxCustomField withValue(Object value) {
        setValue(value);
        return this;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }

    @Override
    public boolean equals(@Nullable Object o) {
        if (this == o) {
            return true;
        }
        if (o == null || getClass() != o.getClass()) {
            return false;
        }
        SandboxCustomField that = (SandboxCustomField) o;
        return Objects.equals(name, that.name) &&
                Objects.equals(value, that.value);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name, value);
    }
}
