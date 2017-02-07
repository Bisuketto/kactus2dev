//-----------------------------------------------------------------------------
// File: VerilogWriterFactory.cpp
//-----------------------------------------------------------------------------
// Project: Kactus2
// Author: Janne Virtanen
// Date: 26.01.2017
//
// Description:
// Creates writes for generating Verilog.
//-----------------------------------------------------------------------------

#include "VerilogWriterFactory.h"

#include <library/LibraryManager/libraryinterface.h>

#include <Plugins/VerilogGenerator/common/WriterGroup.h>
#include <Plugins/VerilogGenerator/CommentWriter/CommentWriter.h>
#include <Plugins/VerilogGenerator/ComponentVerilogWriter/ComponentVerilogWriter.h>
#include <Plugins/VerilogGenerator/ComponentInstanceVerilogWriter/ComponentInstanceVerilogWriter.h>
#include <Plugins/common/PortSorter/InterfaceDirectionNameSorter.h>
#include <Plugins/VerilogGenerator/VerilogHeaderWriter/VerilogHeaderWriter.h>
#include <Plugins/VerilogGenerator/VerilogWireWriter/VerilogWireWriter.h>
#include <Plugins/VerilogGenerator/VerilogAssignmentWriter/VerilogAssignmentWriter.h>
#include <Plugins/VerilogGenerator/PortVerilogWriter/VerilogTopDefaultWriter.h>
#include <Plugins/VerilogGenerator/VerilogInterconnectionWriter/VerilogInterconnectionWriter.h>

#include <Plugins/VerilogImport/VerilogSyntax.h>
#include <Plugins/common/HDLParser/MetaDesign.h>

#include <Plugins/PluginSystem/GeneratorPlugin/FileOutput.h>
#include <Plugins/PluginSystem/GeneratorPlugin/GenerationControl.h>

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::VerilogWriterFactory()
//-----------------------------------------------------------------------------
VerilogWriterFactory::VerilogWriterFactory(LibraryInterface* library,
    MessagePasser* messages, GenerationSettings* settings,
    QString const& kactusVersion, QString const& generatorVersion) :
    library_(library),
    messages_(messages),
    settings_(settings),
    kactusVersion_(kactusVersion),
    generatorVersion_(generatorVersion),
    sorter_(new InterfaceDirectionNameSorter())
{
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::~VerilogWriterFactory()
//-----------------------------------------------------------------------------
VerilogWriterFactory::~VerilogWriterFactory()
{
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::prepareComponent()
//-----------------------------------------------------------------------------
QSharedPointer<GenerationFile> VerilogWriterFactory::prepareComponent(QString const& outputPath,
    QSharedPointer<MetaComponent> component)
{
    // If we are not generating based on a design, we must parse the existing implementation.
    QString implementation;
    QString postModule;
    QString fileName = component->getModuleName() + ".v";
    QString filePath = outputPath + "/" + fileName;
    QString error;

    if (!VerilogSyntax::readImplementation(filePath, implementation, postModule, error))
    {
        messages_->errorMessage(error);

        // If parser says no-go, we dare do nothing.
        return QSharedPointer<VerilogDocument>::QSharedPointer();
    }

    QSharedPointer<VerilogDocument> document = initializeComponentWriters(component);
    document->fileName_ = fileName;
    document->vlnv_ = component->getComponent()->getVlnv().toString();

    // Next comes the implementation.
    QSharedPointer<TextBodyWriter> implementationWriter(new TextBodyWriter(implementation));
    document->topWriter_->setImplementation(implementationWriter);

    // Also write any stuff that comes after the actual module.
    QSharedPointer<TextBodyWriter> postModuleWriter(new TextBodyWriter(postModule));
    document->topWriter_->setPostModule(postModuleWriter);

    return document;
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::prepareDesign()
//-----------------------------------------------------------------------------
QSharedPointer<GenerationFile> VerilogWriterFactory::prepareDesign(QSharedPointer<MetaDesign> design)
{
    QSharedPointer<VerilogDocument> document = initializeComponentWriters(design->getTopInstance());
    document->fileName_ = design->getTopInstance()->getModuleName() + ".v";
    document->vlnv_ = design->getTopInstance()->getComponent()->getVlnv().toString();

    initializeDesignWriters(design, document);

    // Finally, add them to the top writer in desired order.
    addWritersToTopInDesiredOrder(document);

    return document;
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::getLanguage()
//-----------------------------------------------------------------------------
QString VerilogWriterFactory::getLanguage() const
{
    return "Verilog";
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::initializeComponentWriters()
//-----------------------------------------------------------------------------
QSharedPointer<VerilogDocument> VerilogWriterFactory::initializeComponentWriters(QSharedPointer<MetaComponent> topComponent)
{
    QSettings settings;
    QString currentUser = settings.value("General/Username").toString();
    QString componentXmlPath = library_->getPath(topComponent->getComponent()->getVlnv());

    QSharedPointer<VerilogDocument> retval(new VerilogDocument);

    retval->headerWriter_ = QSharedPointer<VerilogHeaderWriter>(new VerilogHeaderWriter(topComponent->getComponent()->getVlnv(), 
        componentXmlPath, currentUser, topComponent->getComponent()->getDescription(), kactusVersion_, generatorVersion_));

    retval->topWriter_ = QSharedPointer<ComponentVerilogWriter>(new ComponentVerilogWriter
        (topComponent, settings_->generateInterfaces_));

    retval->instanceWriters_.clear();

    retval->interconnectionWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->connectionWireWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->adHocWireWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->portWireWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->topAssignmentWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->topDefaultWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    retval->instanceAssignmentWriters_ = QSharedPointer<WriterGroup>(new WriterGroup());

    return retval;
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::initializeDesignWriters()
//-----------------------------------------------------------------------------
void VerilogWriterFactory::initializeDesignWriters(QSharedPointer<MetaDesign> design, QSharedPointer<VerilogDocument> document)
{
    // Comment for top for assignments.
    if (design->getTopInstance()->getPorts()->size() > 0)
    {
        QSharedPointer<CommentWriter> topAssignmentHeaderWriter(
            new CommentWriter("Assignments for the ports of the encompassing component:"));
        topAssignmentHeaderWriter->setIndent(4);
        document->topAssignmentWriters_->add(topAssignmentHeaderWriter);
    }

    // Create assignments fort top ports.
    foreach (QSharedPointer<MetaPort> mPort, *design->getTopInstance()->getPorts())
    {
        if (mPort->downAssignments_.size() > 0)
        {
            foreach (QSharedPointer<MetaPortAssignment> mpa, mPort->downAssignments_)
            {
                QSharedPointer<VerilogAssignmentWriter> topAssignment = QSharedPointer<VerilogAssignmentWriter>
                    (new VerilogAssignmentWriter(mPort->port_->name(), mpa, mPort->port_->getDirection(), true));
                document->topAssignmentWriters_->add(topAssignment);
            }
        }
        else
        {
            QSharedPointer<VerilogTopDefaultWriter> topDefault = QSharedPointer<VerilogTopDefaultWriter>
                (new VerilogTopDefaultWriter(mPort));
            document->topDefaultWriters_->add(topDefault);
        }
    }

    // Create instance writers for the instances.
    foreach(QSharedPointer<MetaInstance> mInstance, *design->getInstances())
    {
        QSharedPointer<ComponentInstance> instance = mInstance->getComponentInstance();

        QSharedPointer<ComponentInstanceVerilogWriter> instanceWriter(new ComponentInstanceVerilogWriter(
            mInstance, sorter_, settings_->generateInterfaces_));

        document->instanceWriters_.append(instanceWriter);

        document->instanceHeaderWriters_.insert(instanceWriter, createHeaderWriterForInstance(mInstance));

        // Comment for instance assignments.
        if (mInstance->getPorts()->size() > 0)
        {
            QSharedPointer<CommentWriter> portWireHeaderWriter(
                new CommentWriter(instance->getInstanceName() + " port wires:"));
            portWireHeaderWriter->setIndent(4);
            document->portWireWriters_->add(portWireHeaderWriter);

            QSharedPointer<CommentWriter> assignmentHeaderWriter(
                new CommentWriter(instance->getInstanceName() + " assignments:"));
            assignmentHeaderWriter->setIndent(4);
            document->instanceAssignmentWriters_->add(assignmentHeaderWriter);
        }

        // Create writers for instance ports, wires, and assignments.
        foreach (QSharedPointer<MetaPort> mPort, *mInstance->getPorts())
        {
            if (mPort->upAssignments_.size() < 1)
            {
                continue;
            }

            QString physName = instance->getInstanceName() + "_" +
                mPort->port_->name();

            document->portWireWriters_->add(QSharedPointer<VerilogWireWriter>(
                new VerilogWireWriter(physName, mPort->vectorBounds_)));

            foreach (QSharedPointer<MetaPortAssignment> mpa, mPort->upAssignments_)
            {
                QSharedPointer<VerilogAssignmentWriter> instanceAssignment = QSharedPointer<VerilogAssignmentWriter>
                    (new VerilogAssignmentWriter(physName, mpa, mPort->port_->getDirection(), false));
                document->instanceAssignmentWriters_->add(instanceAssignment);
            }
        }
    }

    // Create wire writers for the interconnections
    foreach (QSharedPointer<MetaInterconnection> mInterconnect, *design->getInterconnections())
    {
        if (mInterconnect->wires_.size() > 0)
        {
            QSharedPointer<CommentWriter> connectionWireHeaderWriter(
                new CommentWriter(mInterconnect->name_ + " wires:"));
            connectionWireHeaderWriter->setIndent(4);
            document->connectionWireWriters_->add(connectionWireHeaderWriter);
        }

        foreach (QSharedPointer<MetaWire> gw, mInterconnect->wires_)
        {
            document->connectionWireWriters_->add(QSharedPointer<VerilogWireWriter>(new VerilogWireWriter(gw->name_, gw->bounds_)));
        }
    }

    // Create wire writers for the ad-hoc connections
    if (design->getAdHocWires()->size() > 0)
    {
        QSharedPointer<CommentWriter> adHocWireHeaderWriter(
            new CommentWriter("Ad-hoc wires:"));
        adHocWireHeaderWriter->setIndent(4);
       document->adHocWireWriters_->add(adHocWireHeaderWriter);
    }

    foreach (QSharedPointer<MetaWire> adHoc, *design->getAdHocWires())
    {
        document->adHocWireWriters_->add(QSharedPointer<VerilogWireWriter>(new VerilogWireWriter(adHoc->name_, adHoc->bounds_)));
    }
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::addWritersToTopInDesiredOrder()
//-----------------------------------------------------------------------------
void VerilogWriterFactory::addWritersToTopInDesiredOrder(QSharedPointer<VerilogDocument> document) const
{
    document->topWriter_->add(document->interconnectionWriters_);

    document->topWriter_->add(document->connectionWireWriters_);

    document->topWriter_->add(document->adHocWireWriters_);

    document->topWriter_->add(document->portWireWriters_);

    document->topWriter_->add(document->topAssignmentWriters_);

    document->topWriter_->add(document->topDefaultWriters_);

    document->topWriter_->add(document->instanceAssignmentWriters_);

    foreach(QSharedPointer<ComponentInstanceVerilogWriter> instanceWriter, document->instanceWriters_)
    {
        QSharedPointer<Writer> headerWriter = document->instanceHeaderWriters_[instanceWriter];

        QSharedPointer<WriterGroup> instanceGroup(new WriterGroup);
        instanceGroup->add(headerWriter);
        instanceGroup->add(instanceWriter);

        document->topWriter_->add(instanceGroup);
    }
}

//-----------------------------------------------------------------------------
// Function: VerilogWriterFactory::createHeaderWriterForInstance()
//-----------------------------------------------------------------------------
QSharedPointer<Writer> VerilogWriterFactory::createHeaderWriterForInstance(QSharedPointer<MetaInstance> instance) const
{
    QString header = instance->getComponentInstance()->getDescription();
    if (!header.isEmpty())
    {
        header.append("\n");
    }

    header.append("IP-XACT VLNV: " + instance->getComponent()->getVlnv().toString());

    QSharedPointer<CommentWriter> headerWriter(new CommentWriter(header));
    headerWriter->setIndent(4);

    return headerWriter;
}