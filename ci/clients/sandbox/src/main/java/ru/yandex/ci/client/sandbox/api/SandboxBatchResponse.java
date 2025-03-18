
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
 * <p>Java class for SandboxBatchResponse complex type.
 *
 * <p>The following schema fragment specifies the expected content contained within this class.
 *
 * <pre>
 * &lt;complexType name="SandboxBatchResponse">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;sequence>
 *         &lt;element name="status" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="message" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="id" type="{http://www.w3.org/2001/XMLSchema}long"/>
 *       &lt;/sequence>
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "SandboxBatchResponse", propOrder = {
        "status",
        "message",
        "id"
})
@XmlRootElement(name = "sandboxBatchResponse")
public class SandboxBatchResponse implements Serializable {

    @XmlElement(required = true)
    protected SandboxBatchResultStatus status;
    @XmlElement(required = true)
    protected String message;
    protected long id;

    /**
     * Default no-arg constructor
     */
    public SandboxBatchResponse() {
        super();
    }

    /**
     * Fully-initialising value constructor
     */
    public SandboxBatchResponse(final SandboxBatchResultStatus status, final String message, final long id) {
        this.status = status;
        this.message = message;
        this.id = id;
    }

    /**
     * Gets the value of the status property.
     *
     * @return possible object is
     * {@link String }
     */
    public SandboxBatchResultStatus getStatus() {
        return status;
    }

    /**
     * Sets the value of the status property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setStatus(SandboxBatchResultStatus value) {
        this.status = value;
    }

    /**
     * Gets the value of the message property.
     *
     * @return possible object is
     * {@link String }
     */
    public String getMessage() {
        return message;
    }

    /**
     * Sets the value of the message property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setMessage(String value) {
        this.message = value;
    }

    /**
     * Gets the value of the id property.
     */
    public long getId() {
        return id;
    }

    /**
     * Sets the value of the id property.
     */
    public void setId(long value) {
        this.id = value;
    }

    public SandboxBatchResponse withStatus(SandboxBatchResultStatus value) {
        setStatus(value);
        return this;
    }

    public SandboxBatchResponse withMessage(String value) {
        setMessage(value);
        return this;
    }

    public SandboxBatchResponse withId(long value) {
        setId(value);
        return this;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }

}
