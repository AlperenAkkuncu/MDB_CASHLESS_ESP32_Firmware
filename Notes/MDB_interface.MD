![image](https://user-images.githubusercontent.com/95479952/188153109-98838535-f503-4142-9552-c34065e878c9.png)

This diagram shows the valid procedure for a valid single vend.

Every scenerio will be discussed.

## BEGIN SESSION

When begin session command is sent to the VMC, balance should appear on the VMC's screen

There are 3 possible scenarios after begin session is sent by the reader.
1. User can choose a product
2. Session can be cancelled by the reader
3. Session can be cancelled by the VMC
### after begin session
- VMC might send VEND REQUEST command when user selects a product. **(scenario 1)**
- User might push button on the reader to cancel the session, in this case POLL command is responded with SESSION CANCEL **(scenario 2)**
- Uset might cancel the session with escrow button, in that case VMC sends SESSION COMPLETE command. **(scenario 3)**

## VEND REQUEST

This command is sent by the VMC to the reader to notify that the user has selected a product. Vend Request packages holds product number and price information

**VEND REQUEST COMMAND SHOULD ALWAY BE RESPONDED WITH ACK**

- After VEND REQUEST is responded with ACK, vend can be approved with VEND APPROVED package which is given in response to a POLL command by the user **(scenario 1)** .
- VMC can cancel the vending by sending CANCEL VEND command, this might be user pushing the escrow **(scenario 2)** 
- User might have insufficient funds and it that case reader deny the vend b sending VEND DENIED package to VMC **(scenario 3)** 

