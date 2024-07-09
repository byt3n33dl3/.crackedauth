```shell
                   _         _         _   _   
   ___ ___ ___ ___| |_ ___ _| |___ _ _| |_| |_ 
 _|  _|  _| .'|  _| '_| -_| . | .'| | |  _|   |
|_|___|_| |__,|___|_,_|___|___|__,|___|_| |_|_|

        Authentication attack by: pxcs                                               
```

Authentication cracker - trust attack

## 🌟 Features

- [x] Easy to use. Keep your code clear.
- [x] Support CDE modes: Before, Instead, After and Dead.
- [x] Let you modify return value and arguments.
- [x] Support invoking original implementation.
- [x] Remove CDE at any time.

```shell
NSObject *z = NSObject.new;
int(^block)(int x, int y) = ^int(int x, int y) {
    int result = x + y;
    NSLog(@"%d + %d = %d, z is a NSObject: %@", x, y, result, z);
    return result;
};
    
BHToken *token = [block block_hookWithMode:BlockHookModeDead|BlockHookModeBefore|BlockHookModeInstead|BlockHookModeAfter usingBlock:^(BHInvocation *invocation, int x, int y) {
    int ret = 0;
    [invocation getReturnValue:&ret];
    switch (invocation.mode) {
        case BlockHookModeBefore:
            // BHInvocation has to be the first arg.
            NSLog(@"hook before block! invocation:%@", invocation);
            break;
        case BlockHookModeInstead:
            [invocation invokeOriginalBlock];
            NSLog(@"let me see original result: %d", ret);
            // change the block imp and result
            ret = x * y;
            [invocation setReturnValue:&ret];
            NSLog(@"hook instead: '+' -> '*'");
            break;
        case BlockHookModeAfter:
            // print args and result
            NSLog(@"hook after block! %d * %d = %d", x, y, ret);
            break;
        case BlockHookModeDead:
            // BHInvocation is the only arg.
            NSLog(@"block dead! token:%@", invocation.token);
            break;
        default:
            break;
    }
}];
    
NSLog(@"hooked block");
int ret = block(3, 5);
NSLog(@"hooked result:%d", ret);
// remove token.
[token remove];
NSLog(@"remove tokens, original block");
ret = block(3, 5);
NSLog(@"original result:%d", ret);
```