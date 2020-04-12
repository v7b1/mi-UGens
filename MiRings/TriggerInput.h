//
//  TriggerInput.h
//  MiRings
//
//  Created by vb on 17.11.19.
//

#ifndef TriggerInput_h
#define TriggerInput_h

class TriggerInp {
public:
    TriggerInp() { }
    ~TriggerInp() { }
    
    void Init() {
        trigger_ = false;
        previous_trigger_ = false;
    }
    
    void Read(float strum) {
        previous_trigger_ = trigger_;
        trigger_ = strum > 0.0 ;
    }
    
    inline bool rising_edge() const {
        return trigger_ && !previous_trigger_;
    }
    
    inline bool value() const {
        return trigger_;
    }
    
    inline bool DummyRead(float strum) const {
        return strum > 0.0 ;
        //return !GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_2);
    }
    
private:
    bool previous_trigger_;
    bool trigger_;
    
    DISALLOW_COPY_AND_ASSIGN(TriggerInp);
};


#endif /* TriggerInput_h */
