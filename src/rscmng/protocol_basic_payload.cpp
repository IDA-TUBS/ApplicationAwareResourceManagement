// Copyright (C) 2025 IDA 
//
// This file is part of a project licensed under the GNU Lesser General Public License v3.0.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.


#include <rscmng/protocols/basic_payload.hpp>


/*************************************** Basic Protocol **********************************************/
/*
*
*/
rscmng::basic_protocol::BasicPayload::BasicPayload(): 
    object_size(0),
    object_deadline(0)
{}


/*
*
*/
rscmng::basic_protocol::BasicPayload::BasicPayload( 
    uint32_t object_size, 
    std::chrono::nanoseconds object_deadline
):
    object_size(object_size),
    object_deadline(object_deadline)
{}


/*
*
*/
void rscmng::basic_protocol::BasicPayload::serialize(MessageNet_t* rm_payload_message)
{    
    rm_payload_message->add(&object_size, sizeof(object_size));
    rm_payload_message->add(&object_deadline, sizeof(object_deadline));
}


/*
*
*/
void rscmng::basic_protocol::BasicPayload::deserialize(MessageNet_t* rm_payload_message)
{
    rm_payload_message->read(&object_size, sizeof(object_size));
    rm_payload_message->read(&object_deadline, sizeof(object_deadline));
}


/*
*
*/
void rscmng::basic_protocol::BasicPayload::print()
{
   RM_logInfo("Sample size: " << unsigned(object_size))
   RM_logInfo("Sample deadline: " << object_deadline.count() << " ns")
}


// Setter
/*
*
*/
void rscmng::basic_protocol::BasicPayload::set_object_size(uint32_t object_size)
{   
    object_size = object_size;
}


/*
*
*/
void rscmng::basic_protocol::BasicPayload::set_object_deadline(std::chrono::nanoseconds object_deadline)
{
    object_deadline = object_deadline;
}


// Getter
/*
*
*/
uint32_t rscmng::basic_protocol::BasicPayload::get_sample_size()
{
    return object_size;
}


/*
*
*/
std::chrono::nanoseconds rscmng::basic_protocol::BasicPayload::get_sample_deadline()
{
    return object_deadline;
}
/*--------------------------------------------------------------------------------------------------*/

/*------------------------------------------- tx Param ---------------------------------------------*/
rscmng::basic_protocol::txParam::txParam()
:
    fragment_size(0)
{}

rscmng::basic_protocol::txParam::txParam(uint32_t f_size)
:
    fragment_size(f_size)
{}

// Methods
void rscmng::basic_protocol::txParam::serialize(MessageNet_t* param_stream)
{
    param_stream->add(&fragment_size, sizeof(fragment_size));
}

void rscmng::basic_protocol::txParam::deserialize(MessageNet_t* param_stream)
{
    param_stream->read(&fragment_size, sizeof(fragment_size));
}

void rscmng::basic_protocol::txParam::update(MessageNet_t* param_stream)
{
    uint32_t dummy_value;
    param_stream->read(&dummy_value, sizeof(fragment_size));
    if(dummy_value != 0)
    {
        fragment_size = dummy_value;
    }
}

void rscmng::basic_protocol::txParam::print()
{
    RM_logInfo("Fragment size: " << unsigned(fragment_size))
}

// Setter
void rscmng::basic_protocol::txParam::set_fragment_size(uint32_t f_size)
{
    fragment_size = f_size;
}

// Getter
uint32_t rscmng::basic_protocol::txParam::get_fragment_size()
{
    return fragment_size;
}

/*--------------------------------------------------------------------------------------------------*/

/*------------------------------------- Handler Functions ------------------------------------------*/
void* rscmng::basic_protocol::aRM_handler(MessageNet_t* rm_payload_message)
{
    basic_protocol::BasicPayload* qos = new basic_protocol::BasicPayload;

    qos->deserialize(rm_payload_message);

    // Resource Management Magic

    return qos;
}

void* rscmng::basic_protocol::nRM_handler(MessageNet_t* rm_payload_message)
{
    basic_protocol::BasicPayload* qos = new basic_protocol::BasicPayload;

    qos->deserialize(rm_payload_message);

    // Resource Management Magic

    return qos;
}

/*****************************************************************************************************/

/*************************************** Basic Periodic Protocol *****************************************/
rscmng::basic_periodic_protocol::BasicPayload::BasicPayload(): 
    basic_protocol::BasicPayload(),
    sample_period(0)
{}

rscmng::basic_periodic_protocol::BasicPayload::BasicPayload(    
    uint32_t object_size, 
    std::chrono::nanoseconds object_deadline, 
    std::chrono::nanoseconds s_period
):
    basic_protocol::BasicPayload(object_size, object_deadline),
    sample_period(s_period)
{}

// Methods
void rscmng::basic_periodic_protocol::BasicPayload::serialize(MessageNet_t* rm_payload_message)
{
    basic_protocol::BasicPayload::serialize(rm_payload_message);
    rm_payload_message->add(&sample_period, sizeof(sample_period));
}

void rscmng::basic_periodic_protocol::BasicPayload::deserialize(MessageNet_t* rm_payload_message)
{
    basic_protocol::BasicPayload::deserialize(rm_payload_message);
    rm_payload_message->read(&sample_period, sizeof(sample_period));
}

void rscmng::basic_periodic_protocol::BasicPayload::print()
{
    basic_protocol::BasicPayload::print();
    RM_logInfo("Sample period: " << sample_period.count() << " ns")
}

// Setter
void rscmng::basic_periodic_protocol::BasicPayload::set_sample_period(std::chrono::nanoseconds s_period)
{
    sample_period = s_period;
}

// Getter
std::chrono::nanoseconds rscmng::basic_periodic_protocol::BasicPayload::get_sample_period()
{
    return sample_period;
}

/*--------------------------------------------------------------------------------------------------*/

/*------------------------------------------- tx Param ---------------------------------------------*/
rscmng::basic_periodic_protocol::txParam::txParam()
:
    basic_protocol::txParam::txParam(),
    shaping_time(0)
{}

rscmng::basic_periodic_protocol::txParam::txParam(    uint32_t f_size,
                                            std::chrono::nanoseconds t_sh
):
    basic_protocol::txParam::txParam(f_size),
    shaping_time(t_sh)
{}

// Methods
void rscmng::basic_periodic_protocol::txParam::serialize(MessageNet_t* param_stream)
{
    basic_protocol::txParam::serialize(param_stream);
    param_stream->add(&shaping_time, sizeof(shaping_time));
}

void rscmng::basic_periodic_protocol::txParam::deserialize(MessageNet_t* param_stream)
{
    basic_protocol::txParam::deserialize(param_stream);
    param_stream->read(&shaping_time, sizeof(shaping_time));
}

void rscmng::basic_periodic_protocol::txParam::update(MessageNet_t* param_stream)
{
    basic_protocol::txParam::update(param_stream);
    
    std::chrono::nanoseconds dummy_value;
    param_stream->read(&dummy_value, sizeof(shaping_time));
    if(dummy_value != std::chrono::nanoseconds(0))
    {
        shaping_time = dummy_value;
    }
}

void rscmng::basic_periodic_protocol::txParam::print()
{
    basic_protocol::txParam::print();
    RM_logInfo("Sample period: " << shaping_time.count())
}

// Settter
void rscmng::basic_periodic_protocol::txParam::set_shaping_time(std::chrono::nanoseconds t_sh)
{
    shaping_time = t_sh;
}

// Getter
std::chrono::nanoseconds rscmng::basic_periodic_protocol::txParam::get_shaping_time()
{
    return shaping_time;
}

/*--------------------------------------------------------------------------------------------------*/