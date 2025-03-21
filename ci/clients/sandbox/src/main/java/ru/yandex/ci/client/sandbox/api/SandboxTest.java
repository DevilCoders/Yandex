
package ru.yandex.ci.client.sandbox.api;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

import javax.annotation.Nullable;
import javax.xml.bind.annotation.XmlAccessType;
import javax.xml.bind.annotation.XmlAccessorType;
import javax.xml.bind.annotation.XmlElement;
import javax.xml.bind.annotation.XmlRootElement;
import javax.xml.bind.annotation.XmlType;


/**
 * <p>Java class for SandboxTest complex type.
 *
 * <p>The following schema fragment specifies the expected content contained within this class.
 *
 * <pre>
 * &lt;complexType name="SandboxTest">
 *   &lt;complexContent>
 *     &lt;restriction base="{http://www.w3.org/2001/XMLSchema}anyType">
 *       &lt;sequence>
 *         &lt;element name="className" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="methodName" type="{http://www.w3.org/2001/XMLSchema}string"/>
 *         &lt;element name="features" type="{http://www.w3.org/2001/XMLSchema}string" maxOccurs="unbounded" minOccurs="0"/>
 *         &lt;element name="tags" type="{http://www.w3.org/2001/XMLSchema}string" maxOccurs="unbounded" minOccurs="0"/>
 *       &lt;/sequence>
 *     &lt;/restriction>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 */
@XmlAccessorType(XmlAccessType.FIELD)
@XmlType(name = "SandboxTest", propOrder = {
        "className",
        "methodName",
        "features",
        "tags"
})
@XmlRootElement(name = "sandboxTest")
public class SandboxTest implements Serializable {

    @XmlElement(required = true)
    protected String className;
    @XmlElement(required = true)
    protected String methodName;
    protected List<String> features;
    protected List<String> tags;

    /**
     * Default no-arg constructor
     */
    public SandboxTest() {
        super();
    }

    /**
     * Fully-initialising value constructor
     */
    public SandboxTest(final String className, final String methodName, final List<String> features,
                       final List<String> tags) {
        this.className = className;
        this.methodName = methodName;
        this.features = features;
        this.tags = tags;
    }

    /**
     * Gets the value of the className property.
     *
     * @return possible object is
     * {@link String }
     */
    public String getClassName() {
        return className;
    }

    /**
     * Sets the value of the className property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setClassName(String value) {
        this.className = value;
    }

    /**
     * Gets the value of the methodName property.
     *
     * @return possible object is
     * {@link String }
     */
    public String getMethodName() {
        return methodName;
    }

    /**
     * Sets the value of the methodName property.
     *
     * @param value allowed object is
     *              {@link String }
     */
    public void setMethodName(String value) {
        this.methodName = value;
    }

    /**
     * Gets the value of the features property.
     *
     * <p>
     * This accessor method returns a reference to the live list,
     * not a snapshot. Therefore any modification you make to the
     * returned list will be present inside the JAXB object.
     * This is why there is not a <CODE>set</CODE> method for the features property.
     *
     * <p>
     * For example, to add a new item, do as follows:
     * <pre>
     *    getFeatures().add(newItem);
     * </pre>
     *
     *
     * <p>
     * Objects of the following type(s) are allowed in the list
     * {@link String }
     */
    public List<String> getFeatures() {
        if (features == null) {
            features = new ArrayList<String>();
        }
        return this.features;
    }

    /**
     * Gets the value of the tags property.
     *
     * <p>
     * This accessor method returns a reference to the live list,
     * not a snapshot. Therefore any modification you make to the
     * returned list will be present inside the JAXB object.
     * This is why there is not a <CODE>set</CODE> method for the tags property.
     *
     * <p>
     * For example, to add a new item, do as follows:
     * <pre>
     *    getTags().add(newItem);
     * </pre>
     *
     *
     * <p>
     * Objects of the following type(s) are allowed in the list
     * {@link String }
     */
    public List<String> getTags() {
        if (tags == null) {
            tags = new ArrayList<String>();
        }
        return this.tags;
    }

    public SandboxTest withClassName(String value) {
        setClassName(value);
        return this;
    }

    public SandboxTest withMethodName(String value) {
        setMethodName(value);
        return this;
    }

    public SandboxTest withFeatures(@Nullable String... values) {
        if (values != null) {
            for (String value : values) {
                getFeatures().add(value);
            }
        }
        return this;
    }

    public SandboxTest withFeatures(@Nullable Collection<String> values) {
        if (values != null) {
            getFeatures().addAll(values);
        }
        return this;
    }

    public SandboxTest withTags(@Nullable String... values) {
        if (values != null) {
            for (String value : values) {
                getTags().add(value);
            }
        }
        return this;
    }

    public SandboxTest withTags(@Nullable Collection<String> values) {
        if (values != null) {
            getTags().addAll(values);
        }
        return this;
    }

    /**
     * Sets the value of the features property.
     *
     * @param features allowed object is
     *                 {@link String }
     */
    public void setFeatures(List<String> features) {
        this.features = features;
    }

    /**
     * Sets the value of the tags property.
     *
     * @param tags allowed object is
     *             {@link String }
     */
    public void setTags(List<String> tags) {
        this.tags = tags;
    }

}
