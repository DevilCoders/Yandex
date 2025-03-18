
package ru.yandex.ci.client.sandbox.api;

import java.io.Serializable;

import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlType;

import org.apache.commons.lang3.builder.ReflectionToStringBuilder;
import org.apache.commons.lang3.builder.ToStringStyle;

/**
 * <p>Java class for SandboxUpdateTaskBase complex type.
 *
 * <p>The following schema fragment specifies the expected content contained within this class.
 *
 * <pre>
 * &lt;complexType name="SandboxUpdateTaskBase">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;sequence>
 *         &lt;element name="owner" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="kill_timeout" type="{http://www.w3.org/2001/XMLSchema}long"/>
 *         &lt;element name="important" type="{http://www.w3.org/2001/XMLSchema}boolean"/>
 *       &lt;/sequence>
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "SandboxUpdateTaskBase", propOrder = {
        "owner",
        "killTimeout",
        "important"
})
@XmlRootElement(name = "sandboxUpdateTaskBase")
public class SandboxUpdateTaskBase implements Serializable {

    @XmlElement(required = true)
    protected String owner;
    @XmlElement(name = "kill_timeout")
    protected long killTimeout;
    protected boolean important;

    /**
     * Default no-arg constructor
     */
    public SandboxUpdateTaskBase() {
        super();
    }

    /**
     * Fully-initialising value constructor
     */
    public SandboxUpdateTaskBase(final String owner, final long killTimeout, final boolean important) {
        this.owner = owner;
        this.killTimeout = killTimeout;
        this.important = important;
    }

    /**
     * Gets the value of the owner property.
     *
     * @return possible object is
     * {@link String }
     */
    public String getOwner() {
        return owner;
    }

    /**
     * Sets the value of the owner property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setOwner(String value) {
        this.owner = value;
    }

    /**
     * Gets the value of the killTimeout property.
     */
    public long getKillTimeout() {
        return killTimeout;
    }

    /**
     * Sets the value of the killTimeout property.
     */
    public void setKillTimeout(long value) {
        this.killTimeout = value;
    }

    /**
     * Gets the value of the important property.
     */
    public boolean isImportant() {
        return important;
    }

    /**
     * Sets the value of the important property.
     */
    public void setImportant(boolean value) {
        this.important = value;
    }

    public SandboxUpdateTaskBase withOwner(String value) {
        setOwner(value);
        return this;
    }

    public SandboxUpdateTaskBase withKillTimeout(long value) {
        setKillTimeout(value);
        return this;
    }

    public SandboxUpdateTaskBase withImportant(boolean value) {
        setImportant(value);
        return this;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }

}
