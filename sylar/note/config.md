### 模板

#### 特例化
```cpp
template<class T, class FromStr = LexicalCast<std::string, T>, 
                  class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
```
#### 偏特化
```cpp
template<class F, class T>
class LexicalCast
{
public:
    T operator()(const F& v)
    {
        return boost::lexical_cast<T>(v);
    }
};

template<class T>
class LexicalCast<std::string, std::vector<T>>
{
public:
    std::vector<T> operator() (const std::string& v)
    {
        YAML::Node node = YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for (size_t i = 0; i < node.size(); ++i)
        {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::vector<T>, std::string>
{
public:
    std::string operator() (const std::vector<T>& v)
    {
        YAML::Node node;
        for (auto& i : v)
        {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

```
